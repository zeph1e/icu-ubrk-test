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

const char *optlist = "r:w:fvh";
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

static struct option* parse_option(int argc, char *argv[])
{
    static struct option _opt = { DEFAULT_TEST, DEFAULT_RULE, DEFAULT_INPUT, DEFAULT_WIDTH };
    int o;
    char *inf;
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
        case 'h':
        default:
            return 0;
        }
    }

    strncpy(_opt.inputf, (optind >= argc) ? DEFAULT_INPUT : argv[optind], sizeof(_opt.inputf) - 1);
    _opt.inputf[sizeof(_opt.inputf) - 1] = 0;

    LOG("option :\n test type=%s\n rule file=%s\n input file=%s\n line width=%d\n",
            _opt.type == forward ? "forward" : "reverse", _opt.rulef, _opt.inputf, _opt.width);
        
    return &_opt;
}

static void print_usage(const char* pg)
{
    ERR("[Usage] %s [-h] [-f|-v] [-r <RULE_FILE>] [-w <#width>] [INPUT_FILE]\n\n"
        "Options:\n"
        "  -r <RULE_FILE>       specify rule file(default: %s)\n"
        "  -w <width>           line width(default: %d)\n"
        "  -f                   test forward rule(default)\n"
        "  -v                   test reverse rule\n"
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
    return (st.st_size + 1) * sizeof(UChar);
}

inline size_t read_file(const char* file, UChar* buf, size_t bufcnt)
{
    UFILE *ufp;
    size_t readcnt;
    if (!(ufp = u_fopen(file, "r", 0, 0)))
        return 0;
    readcnt = (size_t) u_file_read(buf, bufcnt, ufp);
    buf[readcnt] = 0;
    u_fclose(ufp);
    return readcnt;
}

// test cases
bool test_following(UBreakIterator*,int,UChar*,size_t)
{
}

bool test_preceding(UBreakIterator*,int,UChar*,size_t)
{
}

static bool run_break_test(UBreakIterator* it, enum test_type type, UChar* text, size_t textcnt, int width)
{
    static testcase_t _tc[] = {
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

    rulelen = read_file(opt->rulef, rule, rulesize / 2);
    inputlen = read_file(opt->inputf, input, inputsize / 2);
    LOG("read %lu characters for rule and %lu characters for input\n", rulelen, inputlen);

    it = ubrk_openRules(rule, -1, 0, 0, &parseErr, &status);
    status = U_ZERO_ERROR;
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

    

END:
    if (it) ubrk_close(it);

    if (rule) free(rule);
    if (input) free(input);
}
