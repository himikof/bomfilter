#include <cstdio>
#include <cstring>

namespace
{

struct buffer_view
{
    buffer_view(char* begin, size_t size)
        : begin(begin)
        , pos(begin)
        , limit(begin)
        , end(begin + size)
    {
    }

    char* begin;
    char* pos;
    char* limit;
    char* end;

    bool empty() const {
        return pos == limit;
    }

    void clear() {
        pos = limit = begin;
    }

    void compact() {
        const size_t n = limit - pos;
        memmove(begin, pos, n);
        pos = begin;
        limit = begin + n;
    }
};

const char* begin(buffer_view const& view) {
    return view.pos;
}

const char* end(buffer_view const& view) {
    return view.limit;
}

int write(FILE* f, buffer_view& buf)
{
    size_t written = fwrite(buf.pos, 1, buf.limit - buf.pos, f);
    if (int err = ferror(f))
        return err;
    buf.pos += written;
    return 0;
}

int read(FILE* f, buffer_view& buf)
{
    size_t bytes_read = fread(buf.limit, 1, buf.end - buf.limit, f);
    if (int err = ferror(f))
        return err;
    buf.limit += bytes_read;
    return 0;
}

enum class bom_action_t
{
    ignore,
    ensure,
    strip
};

const char UTF8_BOM[] = { '\xEF', '\xBB', '\xBF' };

struct bom_automaton
{
    bom_automaton(bom_action_t action)
        : bom_bytes_seen(0)
        , action(action)
    {
    }

    size_t bom_bytes_seen;
    bom_action_t action;

    int perform_output() {
        if (action == bom_action_t::ensure) {
            int rv = fwrite(UTF8_BOM, sizeof(UTF8_BOM), 1, stdout);
            if (rv != 1)
                return ferror(stdout);
        }
        return 0;
    }

    bool finished() const {
        return action == bom_action_t::ignore || bom_bytes_seen == sizeof(UTF8_BOM);
    }

    int consume(buffer_view& buf)
    {
        if (finished())
            return 0;

        for (; buf.pos < buf.limit; ++buf.pos) {
            char c = *buf.pos;

            if (c == UTF8_BOM[bom_bytes_seen]) {
                ++bom_bytes_seen;
                if (bom_bytes_seen < sizeof(UTF8_BOM))
                    continue;
                ++buf.pos;
            } else
                bom_bytes_seen = sizeof(UTF8_BOM);

            return perform_output();
        }

        return 0;
    }
};

struct cmdline_t
{
    cmdline_t(size_t argc, const char** argv)
        : errc(0)
        , bom_action(bom_action_t::ignore)
    {
        for (size_t i = 1; i < argc; ++i)
        {
            const char* arg = argv[i];
            if (strcmp(arg, "--bom=ensure") == 0)
                bom_action = bom_action_t::ensure;
            else if (strcmp(arg, "--bom=strip") == 0)
                bom_action = bom_action_t::strip;
            else if (strcmp(arg, "--bom=ignore") == 0)
                bom_action = bom_action_t::ignore;
            else {
                fprintf(stderr, "Unknown argument: %s\n", arg);
                errc = 1;
            }

        }
    }

    int errc;
    bom_action_t bom_action;
};

}

int main(int argc, const char** argv)
{
    cmdline_t cmdline(argc, argv);
    if (cmdline.errc)
        return cmdline.errc;

    static const size_t BUFFER_SIZE = 16 * 1024;
    char buffer_data[BUFFER_SIZE];

    buffer_view buf(buffer_data, BUFFER_SIZE);

    bom_automaton bom_proc(cmdline.bom_action);

    while (!feof(stdin)) {
        if (int err = read(stdin, buf))
            return err;
        if (int err = bom_proc.consume(buf))
            return err;

        if (bom_proc.finished()) {
            if (int err = write(stdout, buf))
                return err;
        }

        buf.clear();
    }

    return 0;
}
