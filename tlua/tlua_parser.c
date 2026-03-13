/*
** tlua_parser.c — TypingLua Transpiler Parser / Code Generator
**
** Stream-based Type Erasure transpiler.
**
** Strategy:
** 1. Read all tokens (including whitespace/comments/newlines).
** 2. Identify type annotation injection points by context:
**    a) After identifier in 'local namelist': skip ': type_expr'
**    b) After identifier in typed funcbody params: skip ': type_expr'
**    c) After ')' in funcbody: skip ': return_typelist'
**    d) Global typed assign 'name: type = expr': skip ': type'
** 3. Emit all non-type tokens verbatim.
** 4. When skipping type annotations, also skip surrounding whitespace
**    that would leave awkward gaps.
*/

#include "tlua_parser.h"

/* ================================================================ */
/* Buffer operations                                                 */
/* ================================================================ */

static void buf_init(TluaBuffer *buf) {
    buf->capacity = 4096;
    buf->data = (char *)malloc(buf->capacity);
    buf->length = 0;
    if (buf->data) buf->data[0] = '\0';
}

static void buf_grow(TluaBuffer *buf, int needed) {
    while (buf->capacity < buf->length + needed + 1) {
        buf->capacity *= 2;
    }
    buf->data = (char *)realloc(buf->data, buf->capacity);
}

static void buf_append(TluaBuffer *buf, const char *str, int len) {
    if (len <= 0) return;
    buf_grow(buf, len);
    memcpy(buf->data + buf->length, str, len);
    buf->length += len;
    buf->data[buf->length] = '\0';
}

static void buf_free(TluaBuffer *buf) {
    free(buf->data);
    buf->data = NULL;
    buf->length = 0;
    buf->capacity = 0;
}

/* ================================================================ */
/* Token stream helpers                                              */
/* ================================================================ */

/* Advance lexer and return current token */
static TluaToken next_token(TluaParser *p) {
    tlua_lexer_next(&p->lexer);
    return p->lexer.token;
}

/* Get current token */
static TluaToken current(TluaParser *p) {
    return p->lexer.token;
}

/* Emit current token text to output buffer */
static void emit_token(TluaParser *p, TluaToken tok) {
    buf_append(&p->output, tok.start, tok.length);
}

/* Skip whitespace/comment/newline tokens, returning the next significant token.
   Does NOT emit the skipped trivia. Returns it in a small buffer for potential emit. */

#define MAX_TRIVIA 256

typedef struct {
    TluaToken tokens[MAX_TRIVIA];
    int count;
} TriviaList;

static TluaToken skip_trivia_collect(TluaParser *p, TriviaList *trivia) {
    trivia->count = 0;
    while (p->lexer.token.type == TK_WHITESPACE ||
           p->lexer.token.type == TK_NEWLINE ||
           p->lexer.token.type == TK_COMMENT) {
        if (trivia->count < MAX_TRIVIA) {
            trivia->tokens[trivia->count++] = p->lexer.token;
        }
        next_token(p);
    }
    return p->lexer.token;
}

static void emit_trivia(TluaParser *p, TriviaList *trivia) {
    for (int i = 0; i < trivia->count; i++) {
        emit_token(p, trivia->tokens[i]);
    }
}

/* Advance past trivia, emitting them */
static void emit_and_skip_trivia(TluaParser *p) {
    while (p->lexer.token.type == TK_WHITESPACE ||
           p->lexer.token.type == TK_NEWLINE ||
           p->lexer.token.type == TK_COMMENT) {
        emit_token(p, p->lexer.token);
        next_token(p);
    }
}

/* Check if token type is a "type starter" — can begin a type_expr */
static int is_type_token(TluaTokenType type) {
    return type == TK_NAME ||      /* identifier-based types: number, string, etc. */
           type == TK_NIL ||       /* nil as type */
           type == TK_FUNCTION ||  /* bare 'function' as type */
           type == TK_FUN ||       /* fun() signature */
           type == TK_LPAREN;      /* parenthesized type */
}

/* Check if a Name token is a type keyword (used in type context) */
static int is_type_keyword_name(TluaToken tok) {
    if (tok.type == TK_NIL) return 1;
    if (tok.type == TK_FUNCTION) return 1;
    if (tok.type == TK_FUN) return 1;
    if (tok.type != TK_NAME) return 0;
    /* Check known type names */
    return tlua_token_is_keyword(&tok, "boolean") ||
           tlua_token_is_keyword(&tok, "number") ||
           tlua_token_is_keyword(&tok, "integer") ||
           tlua_token_is_keyword(&tok, "string") ||
           tlua_token_is_keyword(&tok, "table") ||
           tlua_token_is_keyword(&tok, "thread") ||
           tlua_token_is_keyword(&tok, "userdata") ||
           tlua_token_is_keyword(&tok, "any") ||
           tlua_token_is_keyword(&tok, "void");
}

/* ================================================================ */
/* Type expression skipper                                           */
/* Consumes (skips) a type_expr without emitting anything.           */
/* Returns the token type after the consumed type expression.        */
/* ================================================================ */

static void skip_type_expr(TluaParser *p);

/* Skip a primary type */
static void skip_primary_type(TluaParser *p) {
    TluaToken tok = current(p);

    if (tok.type == TK_LPAREN) {
        /* Parenthesized type: ( type_expr ) */
        next_token(p); /* skip ( */
        /* skip trivia */
        TriviaList trivia;
        skip_trivia_collect(p, &trivia);
        skip_type_expr(p);
        /* skip trivia */
        skip_trivia_collect(p, &trivia);
        if (current(p).type == TK_RPAREN) {
            next_token(p); /* skip ) */
        }
        return;
    }

    if (tok.type == TK_FUN) {
        /* fun(params): ret */
        next_token(p); /* skip 'fun' */
        TriviaList trivia;
        skip_trivia_collect(p, &trivia);
        if (current(p).type == TK_LPAREN) {
            next_token(p); /* skip ( */
            skip_trivia_collect(p, &trivia);
            /* params: name: type, ... */
            while (current(p).type != TK_RPAREN && current(p).type != TK_EOF) {
                /* skip param name */
                if (current(p).type == TK_NAME) {
                    next_token(p);
                    skip_trivia_collect(p, &trivia);
                    if (current(p).type == TK_COLON) {
                        next_token(p); /* skip : */
                        skip_trivia_collect(p, &trivia);
                        skip_type_expr(p);
                    }
                }
                skip_trivia_collect(p, &trivia);
                if (current(p).type == TK_COMMA) {
                    next_token(p); /* skip , */
                    skip_trivia_collect(p, &trivia);
                }
            }
            if (current(p).type == TK_RPAREN) {
                next_token(p); /* skip ) */
            }
        }
        /* optional return type */
        skip_trivia_collect(p, &trivia);
        if (current(p).type == TK_COLON) {
            next_token(p); /* skip : */
            skip_trivia_collect(p, &trivia);
            skip_type_expr(p);
        }
        return;
    }

    /* table<K, V> or bare 'table' */
    if (tok.type == TK_NAME && tlua_token_is_keyword(&tok, "table")) {
        next_token(p); /* skip 'table' */

        /* Save state before consuming trivia — if the next significant
           token is not '<', we must restore so that the trivia is not lost. */
        const char *saved_cur = p->lexer.current;
        int saved_line = p->lexer.line;
        int saved_col = p->lexer.col;
        TluaToken saved_tok = p->lexer.token;
        int saved_has_la = p->lexer.has_lookahead;
        TluaToken saved_la = p->lexer.lookahead;

        TriviaList trivia;
        skip_trivia_collect(p, &trivia);
        if (current(p).type == TK_LT) {
            /* table<K, V> */
            next_token(p); /* skip < */
            skip_trivia_collect(p, &trivia);
            skip_type_expr(p); /* K */
            skip_trivia_collect(p, &trivia);
            if (current(p).type == TK_COMMA) {
                next_token(p); /* skip , */
                skip_trivia_collect(p, &trivia);
                skip_type_expr(p); /* V */
            }
            skip_trivia_collect(p, &trivia);
            if (current(p).type == TK_GT) {
                next_token(p); /* skip > */
            } else if (current(p).type == TK_SHR) {
                /* '>>' was lexed as a single TK_SHR token.
                   We consume one '>' for this generic close and
                   re-lex the remaining '>' so the outer generic
                   can find its own TK_GT. */
                TluaToken shr = current(p);
                /* Fabricate a TK_GT that points to the second '>' */
                p->lexer.token.type = TK_GT;
                p->lexer.token.start = shr.start + 1;
                p->lexer.token.length = 1;
                /* Do NOT call next_token — leave the synthesised '>' as current */
            }
        } else {
            /* Not table<...> — restore trivia so it's not lost */
            p->lexer.current = saved_cur;
            p->lexer.line = saved_line;
            p->lexer.col = saved_col;
            p->lexer.token = saved_tok;
            p->lexer.has_lookahead = saved_has_la;
            p->lexer.lookahead = saved_la;
        }
        return;
    }

    /* base type keyword or identifier */
    if (tok.type == TK_NAME || tok.type == TK_NIL || tok.type == TK_FUNCTION) {
        next_token(p); /* skip the type name */
        return;
    }

    /* If we get here, it's not a recognized type — just return without consuming */
}

/* Skip postfix operations (? and []) */
static void skip_postfix_type(TluaParser *p) {
    skip_primary_type(p);
    for (;;) {
        if (current(p).type == TK_QUESTION) {
            next_token(p); /* skip ? */
        } else if (current(p).type == TK_LBRACKET) {
            /* Check for [] */
            /* We need to peek ahead. Since we can't easily peek in the consumed
               stream, check if next char after [ is ] */
            const char *saved = p->lexer.current;
            int saved_line = p->lexer.line;
            int saved_col = p->lexer.col;
            TluaToken bracket = current(p);
            next_token(p); /* skip [ */
            /* skip any whitespace between [ and ] — though normally none */
            TriviaList trivia;
            skip_trivia_collect(p, &trivia);
            if (current(p).type == TK_RBRACKET) {
                next_token(p); /* skip ] — it's [] */
            } else {
                /* Not [] — this is not a type postfix, restore would be ideal.
                   But since this is type context, it's likely an error.
                   For robustness, just stop the postfix chain. */
                break;
            }
        } else {
            break;
        }
    }
}

/* Skip a complete type_expr (union level).
** IMPORTANT: This function does NOT consume trailing trivia.
** After calling, the current token is the first non-type token. */
static void skip_type_expr(TluaParser *p) {
    skip_postfix_type(p);
    /* Union: | postfix_type ... */
    for (;;) {
        /* Save state before consuming trivia — if the next significant
           token is not '|', we must NOT consume these trivia tokens
           because they belong to the code AFTER the type expression. */
        const char *saved_cur = p->lexer.current;
        int saved_line = p->lexer.line;
        int saved_col = p->lexer.col;
        TluaToken saved_tok = p->lexer.token;
        int saved_has_la = p->lexer.has_lookahead;
        TluaToken saved_la = p->lexer.lookahead;

        TriviaList trivia;
        skip_trivia_collect(p, &trivia);
        if (current(p).type == TK_PIPE) {
            next_token(p); /* skip | */
            /* consume trivia before next type */
            TriviaList trivia2;
            skip_trivia_collect(p, &trivia2);
            skip_postfix_type(p);
        } else {
            /* Restore — the trivia is not part of the type */
            p->lexer.current = saved_cur;
            p->lexer.line = saved_line;
            p->lexer.col = saved_col;
            p->lexer.token = saved_tok;
            p->lexer.has_lookahead = saved_has_la;
            p->lexer.lookahead = saved_la;
            break;
        }
    }
}

/* Skip a return type list: either (T1, T2) or T1, T2 */
static void skip_return_typelist(TluaParser *p) {
    TriviaList trivia;
    if (current(p).type == TK_LPAREN) {
        /* Parenthesized: (T1, T2, ...) */
        next_token(p); /* skip ( */
        skip_trivia_collect(p, &trivia);
        skip_type_expr(p);
        skip_trivia_collect(p, &trivia);
        while (current(p).type == TK_COMMA) {
            next_token(p); /* skip , */
            skip_trivia_collect(p, &trivia);
            skip_type_expr(p);
            skip_trivia_collect(p, &trivia);
        }
        if (current(p).type == TK_RPAREN) {
            next_token(p); /* skip ) */
        }
    } else {
        /* Bare: T1, T2, ... — greedy until non-type token */
        skip_type_expr(p);
        for (;;) {
            /* Save state before consuming trivia, so we can restore
               if the next significant token is not a comma. */
            const char *saved_cur = p->lexer.current;
            int saved_line = p->lexer.line;
            int saved_col = p->lexer.col;
            TluaToken saved_tok = p->lexer.token;
            int saved_has_la = p->lexer.has_lookahead;
            TluaToken saved_la = p->lexer.lookahead;

            TriviaList trivia2;
            skip_trivia_collect(p, &trivia2);
            if (current(p).type == TK_COMMA) {
                /* Peek after comma to see if a type follows */
                /* Save state again for comma rollback */
                const char *saved_cur2 = p->lexer.current;
                int saved_line2 = p->lexer.line;
                int saved_col2 = p->lexer.col;
                TluaToken saved_tok2 = p->lexer.token;
                int saved_has_la2 = p->lexer.has_lookahead;
                TluaToken saved_la2 = p->lexer.lookahead;

                next_token(p); /* skip , */
                TriviaList trivia3;
                skip_trivia_collect(p, &trivia3);

                if (is_type_token(current(p).type) || is_type_keyword_name(current(p))) {
                    /* It's another type in the return list */
                    skip_type_expr(p);
                } else {
                    /* Not a type — restore state to before comma */
                    p->lexer.current = saved_cur2;
                    p->lexer.line = saved_line2;
                    p->lexer.col = saved_col2;
                    p->lexer.token = saved_tok2;
                    p->lexer.has_lookahead = saved_has_la2;
                    p->lexer.lookahead = saved_la2;
                    /* Also restore to before trivia before comma */
                    p->lexer.current = saved_cur;
                    p->lexer.line = saved_line;
                    p->lexer.col = saved_col;
                    p->lexer.token = saved_tok;
                    p->lexer.has_lookahead = saved_has_la;
                    p->lexer.lookahead = saved_la;
                    break;
                }
            } else {
                /* Not a comma — restore trivia so it's not lost */
                p->lexer.current = saved_cur;
                p->lexer.line = saved_line;
                p->lexer.col = saved_col;
                p->lexer.token = saved_tok;
                p->lexer.has_lookahead = saved_has_la;
                p->lexer.lookahead = saved_la;
                break;
            }
        }
    }
}

/* ================================================================ */
/* Annotation skip helpers                                           */
/* These skip ': type_annotation' including surrounding whitespace   */
/* ================================================================ */

/*
** Try to skip a type annotation at the current position.
** Expects current token to be COLON (beginning of ': type').
** Skips: [ws] ':' [ws] type_expr
** Also skips any whitespace that was collected BEFORE the colon.
*/
static void skip_type_annotation(TluaParser *p) {
    /* Current token should be ':' */
    if (current(p).type != TK_COLON) return;
    next_token(p); /* skip : */
    TriviaList trivia;
    skip_trivia_collect(p, &trivia);
    skip_type_expr(p);
}

/* ================================================================ */
/* Main transpilation loop                                           */
/* ================================================================ */

/*
** Process a 'local' statement.
** local NAME [: TYPE] {, NAME [: TYPE]} [= explist]
** local function NAME ...
*/
static void process_local(TluaParser *p) {
    /* Emit 'local' */
    emit_token(p, current(p));
    next_token(p);

    /* Emit trivia between 'local' and next token */
    emit_and_skip_trivia(p);

    /* Check for 'local function' */
    if (current(p).type == TK_FUNCTION) {
        /* local function NAME typed_funcbody */
        emit_token(p, current(p)); /* emit 'function' */
        next_token(p);
        emit_and_skip_trivia(p);

        /* Emit function name */
        if (current(p).type == TK_NAME) {
            emit_token(p, current(p));
            next_token(p);
        }

        /* Process funcbody */
        goto process_funcbody;
    }

    /* local namelist [= explist] */
    /* NAME [: TYPE] */
    if (current(p).type == TK_NAME) {
        emit_token(p, current(p)); /* emit name */
        next_token(p);

        /* Check for : TYPE */
        TriviaList trivia;
        skip_trivia_collect(p, &trivia);
        if (current(p).type == TK_COLON) {
            /* Skip the type annotation (don't emit trivia before colon either) */
            skip_type_annotation(p);
        } else {
            /* Emit collected trivia */
            emit_trivia(p, &trivia);
        }

        /* More names? , NAME [: TYPE] ... */
        while (current(p).type == TK_COMMA) {
            emit_token(p, current(p)); /* emit , */
            next_token(p);
            emit_and_skip_trivia(p);

            if (current(p).type == TK_NAME) {
                emit_token(p, current(p)); /* emit name */
                next_token(p);

                skip_trivia_collect(p, &trivia);
                if (current(p).type == TK_COLON) {
                    skip_type_annotation(p);
                } else {
                    emit_trivia(p, &trivia);
                }
            }
        }
    }

    /* The rest (= explist or end of statement) is handled by main loop */
    return;

process_funcbody:
    /* Process typed funcbody: (params): return_type block end */
    emit_and_skip_trivia(p);

    if (current(p).type == TK_LPAREN) {
        emit_token(p, current(p)); /* emit ( */
        next_token(p);

        /* Process params */
        emit_and_skip_trivia(p);
        while (current(p).type != TK_RPAREN && current(p).type != TK_EOF) {
            if (current(p).type == TK_DOTS) {
                emit_token(p, current(p)); /* emit ... */
                next_token(p);
                emit_and_skip_trivia(p);
                break;
            }
            if (current(p).type == TK_NAME) {
                emit_token(p, current(p)); /* emit param name */
                next_token(p);

                /* Check for : TYPE */
                TriviaList trivia;
                skip_trivia_collect(p, &trivia);
                if (current(p).type == TK_COLON) {
                    skip_type_annotation(p);
                } else {
                    emit_trivia(p, &trivia);
                }
            }

            emit_and_skip_trivia(p);
            if (current(p).type == TK_COMMA) {
                emit_token(p, current(p)); /* emit , */
                next_token(p);
                emit_and_skip_trivia(p);
            }
        }
        if (current(p).type == TK_RPAREN) {
            emit_token(p, current(p)); /* emit ) */
            next_token(p);
        }

        /* Check for : RETURN_TYPE */
        TriviaList trivia;
        skip_trivia_collect(p, &trivia);
        if (current(p).type == TK_COLON) {
            /* Skip return type annotation (don't emit trivia before colon) */
            next_token(p); /* skip : */
            TriviaList trivia2;
            skip_trivia_collect(p, &trivia2);
            skip_return_typelist(p);
        } else {
            emit_trivia(p, &trivia);
        }
    }
}

/*
** Process a 'function' statement (non-local).
** function funcname typed_funcbody
*/
static void process_function(TluaParser *p) {
    /* Emit 'function' */
    emit_token(p, current(p));
    next_token(p);
    emit_and_skip_trivia(p);

    /* Emit funcname: Name {'.' Name} [':' Name] */
    if (current(p).type == TK_NAME) {
        emit_token(p, current(p));
        next_token(p);

        /* dot chain */
        while (current(p).type == TK_DOT) {
            emit_token(p, current(p)); /* emit . */
            next_token(p);
            if (current(p).type == TK_NAME) {
                emit_token(p, current(p)); /* emit name */
                next_token(p);
            }
        }
        /* method colon */
        if (current(p).type == TK_COLON) {
            emit_token(p, current(p)); /* emit : (method) */
            next_token(p);
            if (current(p).type == TK_NAME) {
                emit_token(p, current(p)); /* emit method name */
                next_token(p);
            }
        }
    }

    /* Process funcbody */
    emit_and_skip_trivia(p);

    if (current(p).type == TK_LPAREN) {
        emit_token(p, current(p)); /* emit ( */
        next_token(p);

        /* Process params */
        emit_and_skip_trivia(p);
        while (current(p).type != TK_RPAREN && current(p).type != TK_EOF) {
            if (current(p).type == TK_DOTS) {
                emit_token(p, current(p)); /* emit ... */
                next_token(p);
                emit_and_skip_trivia(p);
                break;
            }
            if (current(p).type == TK_NAME) {
                emit_token(p, current(p)); /* emit param name */
                next_token(p);

                TriviaList trivia;
                skip_trivia_collect(p, &trivia);
                if (current(p).type == TK_COLON) {
                    skip_type_annotation(p);
                } else {
                    emit_trivia(p, &trivia);
                }
            }

            emit_and_skip_trivia(p);
            if (current(p).type == TK_COMMA) {
                emit_token(p, current(p)); /* emit , */
                next_token(p);
                emit_and_skip_trivia(p);
            }
        }
        if (current(p).type == TK_RPAREN) {
            emit_token(p, current(p)); /* emit ) */
            next_token(p);
        }

        /* Check for : RETURN_TYPE */
        TriviaList trivia;
        skip_trivia_collect(p, &trivia);
        if (current(p).type == TK_COLON) {
            next_token(p); /* skip : */
            TriviaList trivia2;
            skip_trivia_collect(p, &trivia2);
            skip_return_typelist(p);
        } else {
            emit_trivia(p, &trivia);
        }
    }
}

/*
** Check if current position is a global typed assignment:
** NAME ':' type '=' explist
** This is tricky because NAME ':' could also be a label or method call.
** We distinguish by context:
** - Label: '::' NAME '::'  (uses TK_DBCOLON)
** - Method call: expr ':' NAME '(' ... ')' — colon is BEFORE name, followed by (
** - Typed global: NAME ':' TYPE '='  — colon is AFTER name, type is followed by '='
**
** Returns 1 if we detected and processed a global typed assignment.
*/
static int try_process_global_typed_assign(TluaParser *p) {
    /* Current is a TK_NAME. We need to look ahead to see if ':' follows,
       then a type, then '='. */

    /* Save full state for potential rollback */
    const char *saved_source_pos = p->lexer.current;
    int saved_line = p->lexer.line;
    int saved_col = p->lexer.col;
    TluaToken saved_tok = p->lexer.token;
    int saved_output_len = p->output.length;
    int saved_has_la = p->lexer.has_lookahead;
    TluaToken saved_la = p->lexer.lookahead;

    TluaToken name_tok = current(p);
    next_token(p);

    /* Skip trivia between name and potential colon */
    TriviaList trivia1;
    skip_trivia_collect(p, &trivia1);

    if (current(p).type != TK_COLON) {
        /* Not a typed assign — restore */
        goto restore;
    }

    /* We have NAME [trivia] COLON. Now check if what follows looks like a type
       followed by '='. We need to tentatively skip the type and check for '='. */
    next_token(p); /* skip : */
    TriviaList trivia2;
    skip_trivia_collect(p, &trivia2);

    /* Check if this looks like a type expression start */
    if (!is_type_token(current(p).type) && !is_type_keyword_name(current(p))) {
        goto restore;
    }

    /* Skip the type expression */
    skip_type_expr(p);

    /* Skip trivia after type */
    TriviaList trivia3;
    skip_trivia_collect(p, &trivia3);

    /* Check for '=' */
    if (current(p).type != TK_ASSIGN) {
        goto restore;
    }

    /* YES — this is a global typed assignment! */
    /* We've already consumed everything up to '='.
       We need to emit: NAME = ... (without the type) */
    /* But we can't just emit from our current position because we already
       advanced past everything. Let's reconstruct the output. */

    /* Reset output to saved length (undo any accidental emit) */
    p->output.length = saved_output_len;
    p->output.data[p->output.length] = '\0';

    /* Emit the name */
    emit_token(p, name_tok);

    /* Emit whitespace before '=' — use trivia3 (between type and =) or just a space */
    if (trivia1.count > 0) {
        emit_trivia(p, &trivia1);
    } else {
        buf_append(&p->output, " ", 1);
    }

    /* Current token is '=' — it will be emitted by the main loop */
    return 1;

restore:
    /* Restore lexer state */
    p->lexer.current = saved_source_pos;
    p->lexer.line = saved_line;
    p->lexer.col = saved_col;
    p->lexer.token = saved_tok;
    p->lexer.has_lookahead = saved_has_la;
    p->lexer.lookahead = saved_la;
    p->output.length = saved_output_len;
    p->output.data[p->output.length] = '\0';
    return 0;
}

/*
** Process anonymous function expression:
** function(typed_params): return_type
** This handles the funcbody part when 'function' appears as an expression value.
*/
static void process_anon_function(TluaParser *p) {
    /* 'function' already emitted or about to be emitted by caller.
       But we handle it here: emit 'function', then typed funcbody. */
    emit_token(p, current(p)); /* emit 'function' */
    next_token(p);
    emit_and_skip_trivia(p);

    if (current(p).type == TK_LPAREN) {
        emit_token(p, current(p)); /* emit ( */
        next_token(p);

        emit_and_skip_trivia(p);
        while (current(p).type != TK_RPAREN && current(p).type != TK_EOF) {
            if (current(p).type == TK_DOTS) {
                emit_token(p, current(p));
                next_token(p);
                emit_and_skip_trivia(p);
                break;
            }
            if (current(p).type == TK_NAME) {
                emit_token(p, current(p));
                next_token(p);

                TriviaList trivia;
                skip_trivia_collect(p, &trivia);
                if (current(p).type == TK_COLON) {
                    skip_type_annotation(p);
                } else {
                    emit_trivia(p, &trivia);
                }
            }

            emit_and_skip_trivia(p);
            if (current(p).type == TK_COMMA) {
                emit_token(p, current(p));
                next_token(p);
                emit_and_skip_trivia(p);
            }
        }
        if (current(p).type == TK_RPAREN) {
            emit_token(p, current(p));
            next_token(p);
        }

        TriviaList trivia;
        skip_trivia_collect(p, &trivia);
        if (current(p).type == TK_COLON) {
            next_token(p);
            TriviaList trivia2;
            skip_trivia_collect(p, &trivia2);
            skip_return_typelist(p);
        } else {
            emit_trivia(p, &trivia);
        }
    }
}

/* ================================================================ */
/* Main transpilation entry                                          */
/* ================================================================ */

static void transpile_stream(TluaParser *p) {
    next_token(p); /* prime the first token */

    while (current(p).type != TK_EOF) {
        TluaToken tok = current(p);

        /* Check for injection points */
        switch (tok.type) {
            case TK_LOCAL:
                process_local(p);
                continue; /* process_local advances tokens */

            case TK_FUNCTION:
                /* Is this a statement-level 'function' (declaration)?
                   Or an expression 'function' (anonymous)?
                   We check: if the previous significant output ended with '='
                   or '(' or ',' then it's an expression context. */
                {
                    /* Look at what comes after: if NAME follows (possibly with dots),
                       it's a declaration. If '(' follows directly, it's anonymous. */
                    /* Save state */
                    const char *saved = p->lexer.current;
                    int saved_line = p->lexer.line;
                    int saved_col = p->lexer.col;
                    TluaToken saved_tok = p->lexer.token;
                    int saved_has_la = p->lexer.has_lookahead;
                    TluaToken saved_la = p->lexer.lookahead;

                    /* Peek ahead past trivia */
                    next_token(p);
                    TriviaList trivia;
                    skip_trivia_collect(p, &trivia);

                    TluaToken after = current(p);

                    /* Restore */
                    p->lexer.current = saved;
                    p->lexer.line = saved_line;
                    p->lexer.col = saved_col;
                    p->lexer.token = saved_tok;
                    p->lexer.has_lookahead = saved_has_la;
                    p->lexer.lookahead = saved_la;

                    if (after.type == TK_LPAREN) {
                        /* Anonymous function */
                        process_anon_function(p);
                    } else {
                        /* Named function declaration */
                        process_function(p);
                    }
                }
                continue;

            case TK_NAME:
                /* Could be a global typed assignment: NAME ':' TYPE '=' ... */
                if (try_process_global_typed_assign(p)) {
                    continue;
                }
                /* Otherwise, just emit the name and continue */
                emit_token(p, tok);
                next_token(p);
                continue;

            default:
                /* Emit everything else verbatim */
                emit_token(p, tok);
                next_token(p);
                continue;
        }
    }
}

/* ================================================================ */
/* Public API                                                        */
/* ================================================================ */

int tlua_transpile(const char *source, const char *filename,
                   char **out_lua, char *error_buf, int error_buf_size) {
    TluaParser parser;
    memset(&parser, 0, sizeof(parser));

    tlua_lexer_init(&parser.lexer, source, filename);
    buf_init(&parser.output);
    parser.filename = filename;
    parser.error_count = 0;
    parser.error_msg[0] = '\0';

    transpile_stream(&parser);

    if (parser.error_count > 0) {
        if (error_buf && error_buf_size > 0) {
            strncpy(error_buf, parser.error_msg, error_buf_size - 1);
            error_buf[error_buf_size - 1] = '\0';
        }
        buf_free(&parser.output);
        *out_lua = NULL;
        return 1;
    }

    *out_lua = parser.output.data; /* caller owns this memory */
    return 0;
}
