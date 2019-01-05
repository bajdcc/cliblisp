//
// Project: CMiniLang
// Author: bajdcc
//

#include <iomanip>
#include <iostream>
#include <algorithm>
#include "cexception.h"
#include "cparser.h"
#include "clexer.h"
#include "cast.h"
#include "cunit.h"

#define TRACE_PARSING 0
#define DUMP_PDA 0
#define DEBUG_AST 0
#define CHECK_AST 0

namespace clib {

    ast_node *cparser::parse(const string_t &str) {
        lexer = std::make_unique<clexer>(str.empty() ? str : (str[0] == '`' ? str : ("(" + str + ")")));
        // 清空词法分析结果
        lexer->reset();
        // 清空AST
        ast.reset();
        // 产生式
        if (unit.get_pda().empty())
            gen();
        // 语法分析（LR）
        program();
        //cast::print2(ast.get_root(), 0, std::cout);
        simplify(ast.get_root());
        return ast.get_root();
    }

    ast_node *cparser::root() const {
        return ast.get_root();
    }

    void cparser::reset() {
        ast_cache_index = 0;
        state_stack.clear();
        ast_stack.clear();
        ast_cache.clear();
        ast_coll_cache.clear();
        ast_reduce_cache.clear();
        state_stack.push_back(0);
    }

    void cparser::next() {
        lexer_t token;
        do {
            token = lexer->next();
            if (token == l_error) {
                auto err = lexer->recent_error();
                printf("[%04d:%03d] %-12s - %s\n",
                       err.line,
                       err.column,
                       ERROR_STRING(err.err).c_str(),
                       err.str.c_str());
            }
        } while (token == l_newline || token == l_space || token == l_error || token == l_comment);
#if 0
        if (token != l_end) {
            qDebug("[%04d:%03d] %-12s - %s\n",
                   lexer->get_last_line(),
                   lexer->get_last_column(),
                   LEX_STRING(lexer->get_type()).c_str(),
                   lexer->current().c_str());
        }
#endif
    }

    ast_node *cparser::simplify(ast_node *node) {
        if (node == nullptr)
            return nullptr;
        auto type = (ast_t) node->flag;
        switch (type) {
            case ast_root: // 根结点，全局声明
                return node->child = simplify(node->child);
            case ast_collection: {
                switch (node->data._coll) {
                    case c_program:
                        if (node->child->data._coll == c_list && node->child->child == node->child->child->next)
                            return simplify(node->child->child);
                        return simplify(node->child);
                    case c_list: {
                        auto i = node->child;
                        if (i) {
                            std::vector<ast_node *> children;
                            children.push_back(i);
                            i = i->next;
                            while (i != node->child) {
                                children.push_back(i);
                                i = i->next;
                            }
                            node->child = nullptr;
                            for (auto &child: children) {
                                cast::set_child(node, simplify(child->child));
                            }
                        }
                        node->flag = ast_sexpr;
                        return node;
                    }
                    case c_sexpr:
                        node->child = simplify(node->child);
                        if (node->child && node->child == node->child->next) {
                            node->child->flag = ast_sexpr;
                            return node->child;
                        }
                        node->flag = ast_sexpr;
                        return node;
                    case c_qexpr: {
                        auto q = node->child->child;
                        if (q->data._coll == c_sexpr) {
                            auto t = simplify(q);
                            t->flag = ast_qexpr;
                            return t;
                        }
                        q = simplify(node->child);
                        node->child = q;
                        node->flag = ast_qexpr;
                        return node;
                    }
                    case c_object:
                        return simplify(node->child);
                    default:
                        break;
                }
            }
                break;
            case ast_string:
            case ast_literal:
            case ast_char:
            case ast_uchar:
            case ast_short:
            case ast_ushort:
            case ast_int:
            case ast_uint:
            case ast_long:
            case ast_ulong:
            case ast_float:
            case ast_double:
                return node;
            default:
                break;
        }
        error("invalid val type");
        return nullptr;
    }

    void cparser::gen() {
#define DEF_OP(name) auto &_##name##_ = unit.token(op_##name)
        DEF_OP(lparan);
        DEF_OP(rparan);
        DEF_OP(quote);
#undef DEF_OP
#define DEF_LEX(name, real) auto &real = unit.token(l_##name)
        DEF_LEX(char, Char);
        DEF_LEX(uchar, UnsignedChar);
        DEF_LEX(short, Short);
        DEF_LEX(ushort, UnsignedShort);
        DEF_LEX(int, Integer);
        DEF_LEX(uint, UnsignedInteger);
        DEF_LEX(long, Long);
        DEF_LEX(ulong, UnsignedLong);
        DEF_LEX(float, Float);
        DEF_LEX(double, Double);
        DEF_LEX(identifier, Identifier);
        DEF_LEX(string, String);
        DEF_LEX(comment, Comment);
        DEF_LEX(space, Space);
        DEF_LEX(newline, Newline);
#undef DEF_LEX
#define DEF_RULE(name) auto &name = unit.rule(#name, c_##name)
        DEF_RULE(program);
        DEF_RULE(list);
        DEF_RULE(sexpr);
        DEF_RULE(qexpr);
        DEF_RULE(object);
#undef DEF_RULE
        program = list | sexpr;
        list = *list + object;
        sexpr = ~_lparan_ + *list + ~_rparan_;
        qexpr = ~_quote_ + object;
        object = Char | UnsignedChar | Short | UnsignedShort | Integer | UnsignedInteger |
                 Long | UnsignedLong | Float | Double | String | Identifier | sexpr | qexpr;
        unit.gen(&program);
#if DUMP_PDA
        unit.dump(std::cout);
#endif
    }

    void check_ast(ast_node *node) {
#if CHECK_AST
        if (node->child) {
            auto &c = node->child;
            auto i = c;
            assert(i->parent == node);
            check_ast(i);
            if (i->next != i) {
                assert(i->prev->next == i);
                assert(i->next->prev == i);
                i = i->next;
                do {
                    assert(i->parent == node);
                    assert(i->prev->next == i);
                    assert(i->next->prev == i);
                    check_ast(i);
                    i = i->next;
                } while (i != c);
            } else {
                assert(i->prev == i);
            }
        }
#endif
    }

    void cparser::program() {
        ast_cache_index = 0;
        state_stack.clear();
        ast_stack.clear();
        ast_cache.clear();
        ast_coll_cache.clear();
        ast_reduce_cache.clear();
        state_stack.push_back(0);
        next();
        auto &pdas = unit.get_pda();
        auto root = ast.new_node(ast_collection);
        root->data._coll = pdas[0].coll;
        cast::set_child(ast.get_root(), root);
        ast_stack.push_back(root);
        std::vector<int> jumps;
        std::vector<int> trans_ids;
        backtrace_t bk_tmp;
        bk_tmp.lexer_index = 0;
        bk_tmp.state_stack = state_stack;
        bk_tmp.ast_stack = ast_stack;
        bk_tmp.current_state = 0;
        bk_tmp.coll_index = 0;
        bk_tmp.reduce_index = 0;
        bk_tmp.direction = b_next;
        std::vector<backtrace_t> bks;
        bks.push_back(bk_tmp);
        auto trans_id = -1;
        while (!bks.empty()) {
            auto bk = &bks.back();
            if (bk->direction == b_success || bk->direction == b_fail) {
                break;
            }
            if (bk->direction == b_fallback) {
                if (bk->trans_ids.empty()) {
                    if (bks.size() > 1) {
                        bks.pop_back();
                        bks.back().direction = b_error;
                        bk = &bks.back();
                    } else {
                        bk->direction = b_fail;
                        continue;
                    }
                }
            }
            ast_cache_index = bk->lexer_index;
            state_stack = bk->state_stack;
            ast_stack = bk->ast_stack;
            auto state = bk->current_state;
            if (bk->direction != b_error)
                for (;;) {
                    auto &current_state = pdas[state];
                    if (lexer->is_type(l_end)) {
                        if (current_state.final) {
                            if (state_stack.empty()) {
                                bk->direction = b_success;
                                break;
                            }
                        } else {
#if TRACE_PARSING
                            std::cout << "parsing unexpected EOF: " << current_state.label << std::endl;
#endif
                            bk->direction = b_error;
                            break;
                        }
                    }
                    auto &trans = current_state.trans;
                    if (trans_id == -1 && !bk->trans_ids.empty()) {
                        trans_id = bk->trans_ids.back() & ((1 << 16) - 1);
                        bk->trans_ids.pop_back();
                    } else {
                        trans_ids.clear();
                        for (auto i = 0; i < trans.size(); ++i) {
                            auto &cs = trans[i];
                            if (valid_trans(cs))
                                trans_ids.push_back(i | pda_edge_priority(cs.type) << 16);
                        }
                        if (!trans_ids.empty()) {
                            std::sort(trans_ids.begin(), trans_ids.end(), std::greater<>());
                            if (trans_ids.size() > 1) {
                                bk_tmp.lexer_index = ast_cache_index;
                                bk_tmp.state_stack = state_stack;
                                bk_tmp.ast_stack = ast_stack;
                                bk_tmp.current_state = state;
                                bk_tmp.trans_ids = trans_ids;
                                bk_tmp.coll_index = ast_coll_cache.size();
                                bk_tmp.reduce_index = ast_reduce_cache.size();
                                bk_tmp.direction = b_next;
#if DEBUG_AST
                                for (auto i = 0; i < bks.size(); ++i) {
                                    auto &_bk = bks[i];
                                    printf("[DEBUG] Branch old: i=%d, LI=%d, SS=%d, AS=%d, S=%d, TS=%d, CI=%d, RI=%d, TK=%d\n",
                                           i, _bk.lexer_index, _bk.state_stack.size(),
                                           _bk.ast_stack.size(), _bk.current_state, _bk.trans_ids.size(),
                                           _bk.coll_index, _bk.reduce_index, _bk.ast_ids.size());
                                }
#endif
                                bks.push_back(bk_tmp);
                                bk = &bks.back();
#if DEBUG_AST
                                printf("[DEBUG] Branch new: BS=%d, LI=%d, SS=%d, AS=%d, S=%d, TS=%d, CI=%d, RI=%d, TK=%d\n",
                                       bks.size(), bk_tmp.lexer_index, bk_tmp.state_stack.size(),
                                       bk_tmp.ast_stack.size(), bk_tmp.current_state, bk_tmp.trans_ids.size(),
                                       bk_tmp.coll_index, bk_tmp.reduce_index, bk_tmp.ast_ids.size());
#endif
                                bk->direction = b_next;
                                break;
                            } else {
                                trans_id = trans_ids.back() & ((1 << 16) - 1);
                                trans_ids.pop_back();
                            }
                        } else {
#if TRACE_PARSING
                            std::cout << "parsing error: " << current_state.label << std::endl;
#endif
                            bk->direction = b_error;
                            break;
                        }
                    }
                    auto &t = trans[trans_id];
                    if (t.type == e_finish) {
                        if (!lexer->is_type(l_end)) {
#if TRACE_PARSING
                            std::cout << "parsing redundant code: " << current_state.label << std::endl;
#endif
                            bk->direction = b_fail;
                            break;
                        }
                    }
                    auto jump = trans[trans_id].jump;
#if TRACE_PARSING
                    printf("State: %3d => To: %3d   -- Action: %-10s -- Rule: %s\n",
                           state, jump, pda_edge_str(t.type).c_str(), current_state.label.c_str());
#endif
                    do_trans(state, *bk, trans[trans_id]);
                    state = jump;
                }
            if (bk->direction == b_error) {
#if DEBUG_AST
                for (auto i = 0; i < bks.size(); ++i) {
                    auto &_bk = bks[i];
                    printf("[DEBUG] Backtrace failed: i=%d, LI=%d, SS=%d, AS=%d, S=%d, TS=%d, CI=%d, RI=%d, TK=%d\n",
                           i, _bk.lexer_index, _bk.state_stack.size(),
                           _bk.ast_stack.size(), _bk.current_state, _bk.trans_ids.size(),
                           _bk.coll_index, _bk.reduce_index, _bk.ast_ids.size());
                }
#endif
                for (auto &i : bk->ast_ids) {
                    auto &token = ast_cache[i];
                    check_ast(token);
#if DEBUG_AST
                    printf("[DEBUG] Backtrace failed, unlink token: %p, PB=%p\n", token, token->parent);
#endif
                    cast::unlink(token);
                    check_ast(token);
                }
                auto size = ast_reduce_cache.size();
                for (auto i = size; i > bk->reduce_index; --i) {
                    auto &coll = ast_reduce_cache[i - 1];
                    check_ast(coll);
#if DEBUG_AST
                    printf("[DEBUG] Backtrace failed, unlink: %p, PB=%p, NE=%d, CB=%d\n",
                           coll, coll->parent, cast::children_size(coll->parent), cast::children_size(coll));
#endif
                    cast::unlink(coll);
                    check_ast(coll);
                }
                ast_reduce_cache.erase(ast_reduce_cache.begin() + bk->reduce_index, ast_reduce_cache.end());
                size = ast_coll_cache.size();
                for (auto i = size; i > bk->coll_index; --i) {
                    auto &coll = ast_coll_cache[i - 1];
                    assert(coll->flag == ast_collection);
                    check_ast(coll);
#if DEBUG_AST
                    printf("[DEBUG] Backtrace failed, delete coll: %p, PB=%p, CB=%p, NE=%d, CS=%d\n",
                           coll, coll->parent, coll->child,
                           cast::children_size(coll->parent), cast::children_size(coll));
#endif
                    cast::unlink(coll);
                    check_ast(coll);
                    ast.remove(coll);
                }
                ast_coll_cache.erase(ast_coll_cache.begin() + bk->coll_index, ast_coll_cache.end());
                bk->direction = b_fallback;
            }
            trans_id = -1;
        }
    }

    ast_node *cparser::terminal() {
        if (lexer->is_type(l_end)) { // 结尾
            error("unexpected token EOF of expression");
        }
        if (ast_cache_index < ast_cache.size()) {
            return ast_cache[ast_cache_index++];
        }
        if (lexer->is_type(l_operator)) {
            auto node = ast.new_node(ast_operator);
            node->data._op = lexer->get_operator();
            match_operator(node->data._op);
            ast_cache.push_back(node);
            ast_cache_index++;
            return node;
        }
        if (lexer->is_type(l_keyword)) {
            auto node = ast.new_node(ast_keyword);
            node->data._keyword = lexer->get_keyword();
            match_keyword(node->data._keyword);
            ast_cache.push_back(node);
            ast_cache_index++;
            return node;
        }
        if (lexer->is_type(l_identifier)) {
            auto node = ast.new_node(ast_literal);
            ast.set_str(node, lexer->get_identifier());
            match_type(l_identifier);
            ast_cache.push_back(node);
            ast_cache_index++;
            return node;
        }
        if (lexer->is_number()) {
            ast_node *node = nullptr;
            auto type = lexer->get_type();
            switch (type) {
#define DEFINE_NODE_INT(t) \
            case l_##t: \
                node = ast.new_node(ast_##t); \
                node->data._##t = lexer->get_##t(); \
                break;
                DEFINE_NODE_INT(char)
                DEFINE_NODE_INT(uchar)
                DEFINE_NODE_INT(short)
                DEFINE_NODE_INT(ushort)
                DEFINE_NODE_INT(int)
                DEFINE_NODE_INT(uint)
                DEFINE_NODE_INT(long)
                DEFINE_NODE_INT(ulong)
                DEFINE_NODE_INT(float)
                DEFINE_NODE_INT(double)
#undef DEFINE_NODE_INT
                default:
                    error("invalid number");
                    break;
            }
            match_number();
            ast_cache.push_back(node);
            ast_cache_index++;
            return node;
        }
        if (lexer->is_type(l_string)) {
            std::stringstream ss;
            ss << lexer->get_string();
#if 0
            printf("[%04d:%03d] String> %04X '%s'\n", clexer->get_line(), clexer->get_column(), idx, clexer->get_string().c_str());
#endif
            match_type(l_string);

            while (lexer->is_type(l_string)) {
                ss << lexer->get_string();
#if 0
                printf("[%04d:%03d] String> %04X '%s'\n", clexer->get_line(), clexer->get_column(), idx, clexer->get_string().c_str());
#endif
                match_type(l_string);
            }
            auto node = ast.new_node(ast_string);
            ast.set_str(node, ss.str());
            ast_cache.push_back(node);
            ast_cache_index++;
            return node;
        }
        error("invalid type");
        return nullptr;
    }

    bool cparser::valid_trans(const pda_trans &trans) const {
        auto &la = trans.LA;
        if (!la.empty()) {
            auto success = false;
            for (auto &_la : la) {
                if (LA(_la)) {
                    success = true;
                    break;
                }
            }
            if (!success)
                return false;
        }
        switch (trans.type) {
            case e_shift:
                break;
            case e_pass:
                break;
            case e_move:
                break;
            case e_left_recursion:
                break;
            case e_reduce: {
                if (state_stack.empty())
                    return false;
                if (trans.status != state_stack.back())
                    return false;
            }
                break;
            case e_finish:
                break;
            default:
                break;
        }
        return true;
    }

    void cparser::do_trans(int state, backtrace_t &bk, const pda_trans &trans) {
        switch (trans.type) {
            case e_shift: {
                state_stack.push_back(state);
                auto new_node = ast.new_node(ast_collection);
                auto &pdas = unit.get_pda();
                new_node->data._coll = pdas[trans.jump].coll;
#if DEBUG_AST
                printf("[DEBUG] Shift: top=%p, new=%p, CS=%d\n", ast_stack.back(), new_node,
                       cast::children_size(ast_stack.back()));
#endif
                ast_coll_cache.push_back(new_node);
                ast_stack.push_back(new_node);
            }
                break;
            case e_pass: {
                bk.ast_ids.insert(ast_cache_index);
                terminal();
#if CHECK_AST
                check_ast(t);
#endif
#if DEBUG_AST
                printf("[DEBUG] Move: parent=%p, child=%p, CS=%d\n", ast_stack.back(), t,
                       cast::children_size(ast_stack.back()));
#endif
            }
                break;
            case e_move: {
                bk.ast_ids.insert(ast_cache_index);
                auto t = terminal();
#if CHECK_AST
                check_ast(t);
#endif
#if DEBUG_AST
                printf("[DEBUG] Move: parent=%p, child=%p, CS=%d\n", ast_stack.back(), t,
                       cast::children_size(ast_stack.back()));
#endif
                cast::set_child(ast_stack.back(), t);
            }
                break;
            case e_left_recursion:
                break;
            case e_reduce: {
                auto new_ast = ast_stack.back();
                check_ast(new_ast);
                if (new_ast->flag != ast_collection) {
                    bk.ast_ids.insert(ast_cache_index);
                }
                state_stack.pop_back();
                ast_stack.pop_back();
                ast_reduce_cache.push_back(new_ast);
#if DEBUG_AST
                printf("[DEBUG] Reduce: parent=%p, child=%p, CS=%d, AS=%d, RI=%d\n",
                       ast_stack.back(), new_ast, cast::children_size(ast_stack.back()),
                       ast_stack.size(), ast_reduce_cache.size());
#endif
                cast::set_child(ast_stack.back(), new_ast);
                check_ast(ast_stack.back());
            }
                break;
            case e_finish:
                state_stack.pop_back();
                break;
        }
    }

    bool cparser::LA(struct unit *u) const {
        if (u->t != u_token)
            return false;
        auto token = to_token(u);
        if (ast_cache_index < ast_cache.size()) {
            auto &cache = ast_cache[ast_cache_index];
            if (token->type == l_keyword)
                return cache->flag == ast_keyword && cache->data._keyword == token->value.keyword;
            if (token->type == l_operator)
                return cache->flag == ast_operator && cache->data._op == token->value.op;
            return cast::ast_equal((ast_t) cache->flag, token->type);
        }
        if (token->type == l_keyword)
            return lexer->is_keyword(token->value.keyword);
        if (token->type == l_operator)
            return lexer->is_operator(token->value.op);
        return lexer->is_type(token->type);
    }

    void cparser::expect(bool flag, const string_t &info) {
        if (!flag) {
            error(info);
        }
    }

    void cparser::match_keyword(keyword_t type) {
        expect(lexer->is_keyword(type), string_t("expect keyword ") + KEYWORD_STRING(type));
        next();
    }

    void cparser::match_operator(operator_t type) {
        expect(lexer->is_operator(type), string_t("expect operator " + OPERATOR_STRING(type)));
        next();
    }

    void cparser::match_type(lexer_t type) {
        expect(lexer->is_type(type), string_t("expect type " + LEX_STRING(type)));
        next();
    }

    void cparser::match_number() {
        expect(lexer->is_number(), "expect number");
        next();
    }

    void cparser::match_integer() {
        expect(lexer->is_integer(), "expect integer");
        next();
    }

    void cparser::error(const string_t &info) {
        std::stringstream ss;
        ss << '[' << std::setfill('0') << std::setw(4) << lexer->get_line();
        ss << ':' << std::setfill('0') << std::setw(3) << lexer->get_column();
        ss << ']' << " PARSER ERROR: " << info;
        throw cexception(ss.str());
    }
}
