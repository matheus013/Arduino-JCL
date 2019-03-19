#ifndef HELPERS_H
#define HELPERS_H

#define ATTRIBUTE_OBJECT(type, name) \
    private: \
        type m_##name; \
    public: \
        type get_##name () const { \
            return m_##name ; \
        } \
        bool set_##name (type name) { \
            bool ret = false; \
            if ((ret = m_##name != name)) { \
                m_##name = name; \
            } \
            return ret; \
        } \
    private:

#define ATTRIBUTE_BOOL(name) \
    private: \
        bool m_##name; \
    public: \
        bool is_##name () const { \
            return m_##name ; \
        } \
        bool set_##name (bool name) { \
            bool ret = false; \
            if ((ret = m_##name != name)) { \
                m_##name = name; \
            } \
            return ret; \
        } \
    private:


#endif // HELPERS_H