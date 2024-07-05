#include <string_view>

class StringTokenizer
{
private:
    using size_type = std::string_view::size_type;

    std::string_view m_string;
    size_type m_index = 0;
    size_type m_next = 0;
    char m_delimiter = '\n';

public:
    StringTokenizer(const std::string_view &string, char delimiter) : m_string(string), m_delimiter(delimiter)
    {
    }

    std::string_view next()
    {
        std::string_view next(m_string.begin() + m_index, m_next - m_index);
        m_index = m_next + 1;

        return next;
    }

    bool has_next()
    {
        for (size_type i = m_index; i <= m_string.length(); i++)
        {
            if (m_string[i] == m_delimiter)
            {
                if (i == 0) {
                    ++m_index;
                    continue;
                }

                m_next = i;
                return true;
            }else {
                if (i == m_string.length() && m_index != m_string.length()) {
                    m_next = i;
                    return true;
                }
            }
        }

        return false;
    }

    void reset()
    {
        m_index = 0;
        m_next = 0;
    }
};