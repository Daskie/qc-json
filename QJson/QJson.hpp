#pragma once



#include <string_view>
#include <fstream>
#include <sstream>
#include <vector>



namespace qjson {

    using std::pair;
    using std::string;
    using std::string_view;
    using std::move;

    inline pair<bool, string> readTextFile(string_view file) {
        std::ifstream ifs(file.data());
        if (!ifs.good()) {
            return {};
        }
        string str(std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>{});
        if (!ifs.good()) {
            return {};
        }
        return {true, move(str)};
    }

    inline bool writeTextFile(string_view file, string_view str) {
        std::ofstream ofs(file.data());
        if (!ofs.good()) {
            return false;
        }
        ofs << str;
        return ofs.good();
    }

    class Writer {

        public:

        Writer(bool compact) :
            m_ss(),
            m_state(),
            m_indent(0)
        {
            m_ss << '{';
            m_state.push_back(State{false, compact, false});
        }

        Writer(const Writer & other) :
            m_ss(),
            m_state(other.m_state),
            m_indent(other.m_indent)
        {
            m_ss << other.m_ss.rdbuf();
        }

        Writer(Writer && other) :
            m_ss(move(other.m_ss)),
            m_state(move(other.m_state)),
            m_indent(other.m_indent)
        {
            other.m_indent = 0;
        }

        Writer & operator=(const Writer & other) = delete;
        Writer & operator=(Writer && other) = delete;

        void startObject(string_view name, bool compact) {
            start(name, compact, '{');
        }

        void startArray(string_view name, bool compact) {
            start(name, compact, '[');
        }

        void put(string_view name, string_view str) {
            prefix(name);
            m_ss << '"' << str << '"';
            m_state.back().content = true;
        }

        void put(string_view str) {
            prefix();
            m_ss << '"' << str << '"';
            m_state.back().content = true;
        }

        void end() {
            const State & state(m_state.back());
            if (state.compact) {
                if (state.content) {
                    m_ss << ' ';
                }
            }
            else {
                m_ss << '\n';
                --m_indent;
                indent();
            }
            m_ss << (state.array ? ']' : '}');
            m_state.pop_back();
        }

        string finish() {
            if (m_state.size() > 1) {
                // TODO ERROR
            }
            else if (m_state.size() == 1) {
                end();
            }
            return m_ss.str();
        }

        bool finish(string_view file) {
            return writeTextFile(file, finish());
        }

        private:

        void start(string_view name, bool compact, char bracket) {
            prefix(name);
            m_ss << bracket;
            m_state.back().content = true;
            m_state.push_back(State{bracket == '[', m_state.back().compact || compact, false});
        }

        void prefix() {
            const State & state(m_state.back());
            if (state.content) {
                m_ss << ',';
            }
            if (state.compact) {
                m_ss << ' ';
            }
            else {
                m_ss << '\n';
                m_indent += !state.content;
                indent();
            }
        }

        void prefix(string_view name) {
            prefix();
            m_ss << '"' << name << "\": ";
        }

        void indent() {
            for (int i(0); i < m_indent; ++i) {
                m_ss << "    ";
            }
        }

        struct State { bool array, compact, content; };

        std::stringstream m_ss;
        std::vector<State> m_state;
        int m_indent;

    };

}