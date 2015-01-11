#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <stdbool.h>
#include <unicode/ubrk.h>
#include <linux/limits.h>

#include <unicode/ustdio.h>
#include <unicode/ubrk.h>
#include <execinfo.h>
#include <signal.h>

#define DEBUG           1
#define LINE_OVERFLOW   1

#if defined(DEBUG) && DEBUG
#define LOG(...)  OUT(__VA_ARGS__)
#else
#define LOG(...)
#endif

#define OUT(...)  printf(__VA_ARGS__)
#define ERR(...)  fprintf(stderr, __VA_ARGS__)

#define DEFAULT_RULE   "rule.line"
#define DEFAULT_INPUT  "input"
#define DEFAULT_WIDTH   40
#define DEFAULT_TEST    0

const char *optlist = "r:w:fvhs";
enum test_type {
    forward = 0,
    reverse,
    safe_forward,
    safe_reverse,

    test_type_max
};

struct option {
    enum test_type type;
    char rulef[PATH_MAX];
    char inputf[PATH_MAX];
    int width;
};

typedef bool (*testcase_t)(UBreakIterator*,int width, UChar* text, size_t textcnt);

#define BT_MAX_DEPTH    256

void __print_backtrace(void)
{
    void * btbuf[BT_MAX_DEPTH];
    int depth;
    char **symbols;
    int i;

    depth = backtrace(btbuf, BT_MAX_DEPTH);
    fprintf(stderr, "Obtained %d frames\n", depth-1);

    symbols = (char**)backtrace_symbols(btbuf, depth);

    for (i = 1; i < depth; i++) {
        fprintf(stderr, "#%3d : %s\n", i, symbols[i]);
    }

    free(symbols);
}

static void __sig_segfault(int s)
{
    __print_backtrace();
    exit(1);
}
// register stack trace on segfault
__attribute__ ((__constructor__))
static void __register_stack_tracer()
{
    signal(SIGSEGV, __sig_segfault);
}


static struct option* parse_option(int argc, char *argv[])
{
    static struct option _opt = { DEFAULT_TEST, DEFAULT_RULE, DEFAULT_INPUT, DEFAULT_WIDTH };
    int o;
    char *inf;
    bool safe = false;
    while ((o = getopt(argc, argv, optlist)) != -1) {
        switch (o) {
        case 'w':
            _opt.width = atoi(optarg);
            break;
        case 'r':
            strncpy(_opt.rulef, optarg, sizeof(_opt.rulef) - 1);
            _opt.rulef[sizeof(_opt.rulef) - 1] = 0;
            break;
        case 'v':
            _opt.type = reverse;
            break;
        case 's':
            safe = true;
            break;
        case 'f':
            continue;
        case 'h':
        default:
            return 0;
        }
    }

    if (safe) _opt.type += safe_forward;

    strncpy(_opt.inputf, (optind >= argc) ? DEFAULT_INPUT : argv[optind], sizeof(_opt.inputf) - 1);
    _opt.inputf[sizeof(_opt.inputf) - 1] = 0;

    LOG("option :\n test type=%s%s\n rule file=%s\n input file=%s\n line width=%d\n",
            _opt.type > reverse ? "safe_" : "", (_opt.type % 2) == forward ? "forward" : "reverse",
            _opt.rulef, _opt.inputf, _opt.width);

    return &_opt;
}

static void print_usage(const char* pg)
{
    ERR("[Usage] %s [-hs] [-f|-v] [-r <RULE_FILE>] [-w <#width>] [INPUT_FILE]\n\n"
        "Options:\n"
        "  -r <RULE_FILE>       specify rule file(default: %s)\n"
        "  -w <width>           line width(default: %d)\n"
        "  -f                   test forward rule(default)\n"
        "  -v                   test reverse rule\n"
        "  -s                   test safe (forward|reverse) rule\n"
        "  -h                   print this help\n"
        "\n"
        "If [INPUT_FILE] is not specified, \"input\" file is used as an input\n"
            , pg, DEFAULT_RULE, DEFAULT_WIDTH);

}

inline size_t get_safe_bufsize(const char* file)
{
    struct stat st;
    if (stat(file, &st) == -1) {
        perror(file);
        return 0;
    }
    LOG("%s: file=%s, size=%d, returns=%d\n",
        __FUNCTION__, file, st.st_size, (st.st_size + 1) * sizeof(UChar));
    return (st.st_size + 1) * sizeof(UChar);
}

inline size_t read_file(const char* file, UChar* buf, size_t bufcnt)
{
    UFILE *ufp;
    size_t readcnt;
    if (!(ufp = u_fopen(file, "r", 0, "UTF-8")))
        return 0;
    readcnt = (size_t) u_file_read(buf, bufcnt, ufp);
    buf[readcnt] = 0;
    int i;
#if 0
    for (i = 0; i < readcnt; i++) {
        printf("%02x%02x", buf[i] & 0x00FF, (buf[i] & 0xFF00) >> 8);
    }
    puts("");
#endif
    u_fclose(ufp);
    return readcnt;
}

// test cases
bool test_next(UBreakIterator* it, int width, UChar* text, size_t textcnt)
{
}

bool test_previous(UBreakIterator* it, int width, UChar* text, size_t textcnt)
{
}

bool test_following(UBreakIterator* it, int width, UChar* text, size_t textcnt)
{
}

bool test_preceding(UBreakIterator* it, int width, UChar* text, size_t textcnt)
{
}

static bool run_break_test(UBreakIterator* it, enum test_type type, int width, UChar* text, size_t textcnt)
{
    static testcase_t _tc[] = {
        &test_next,
        &test_previous,
        &test_following,
        &test_preceding,
    };
    return _tc[type](it, width, text, textcnt);
}

int main(int argc, char* argv[])
{
    struct option *opt;
    UChar *rule, *input;
    size_t rulesize, inputsize;
    size_t rulelen, inputlen;
    bool result;

    UParseError parseErr;
    UErrorCode status;
    UBreakIterator *it = 0;

    opt = parse_option(argc, argv);
    if (!opt) {
        print_usage(argv[0]);
        return 1;
    }

    rulesize = get_safe_bufsize(opt->rulef);
    inputsize = get_safe_bufsize(opt->inputf);

    rule = (UChar*) malloc(rulesize);
    input = (UChar*) malloc(inputsize);
    if (!rule || !input) {
        ERR("Unable to allocate memory\n");
        goto END;
    }
    LOG("allocated %lu bytes for rule and %lu bytes for input\n", rulesize, inputsize);

    rulelen = read_file(opt->rulef, rule, (rulesize / 2) - 1);
    inputlen = read_file(opt->inputf, input, (inputsize / 2 ) - 1);
    LOG("read %lu characters for rule and %lu characters for input\n", rulelen, inputlen);

    status = U_ZERO_ERROR;
    it = ubrk_openRules(rule, -1, 0, -1, &parseErr, &status);
    if (U_FAILURE(status)) {
        ERR("Unable to open break iterator: %s(%d)\n", u_errorName(status), status);
        LOG("Parse error at line=%d offset=%d\n", parseErr.line, parseErr.offset);
        goto END;
    }
    status = U_ZERO_ERROR;
    ubrk_setText(it, input, -1, &status);
    if (U_FAILURE(status)) {
        ERR("Unable to set text: %s(%d)\n", u_errorName(status), status);
        goto END;
    }

    result = run_break_test(it, opt->type, opt->width, input, inputlen);



END:
    if (it) ubrk_close(it);

    if (rule) free(rule);
    if (input) free(input);
}
