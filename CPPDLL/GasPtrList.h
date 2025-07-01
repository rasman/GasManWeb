#ifndef GASPTRLIST_H
#define GASPTRLIST_H

#include <vector>
#include <iostream>
#include <cstdint>

// The GasPtrList class is a template class that provides a list with an auto-delete feature

// If auto-deleting is turned on, all the items in a collection are deleted 
// when the collection itself is deleted. The default setting is true

template <typename T>
class GasPtrList : public std::vector<T *>
{
public:
    GasPtrList() : std::vector<T *>(), autoDelete(true) {}
    GasPtrList(const GasPtrList<T *> &other) : std::vector<T *>(other), autoDelete(true) {}
    ~GasPtrList() { clear(); }

    inline void setAutoDelete(bool del) { autoDelete = del; }
    inline bool isAutoDelete() const { return autoDelete; }

    void Serialize(std::iostream &stream, bool storing, uint32_t serializationFlags = 0)
    {
        if (storing) {
            int m_nSize = GasPtrList::size();
            stream << m_nSize;
            for (int i = 0; i < m_nSize; ++i)
                GasPtrList::at(i)->Serialize(stream, storing, serializationFlags);
        } else {
            int m_nSize;
            stream >> m_nSize;
            for (int i = 0; i < m_nSize; ++i) {
                GasPtrList::push_back(new T);
                GasPtrList::at(i)->Serialize(stream, storing, serializationFlags);
            }
        }
    }

    void clear() {
        if (!autoDelete) {
            std::vector<T *>::clear();
            return;
        }
        for (auto ptr : *this) {
            delete ptr;
        }
        std::vector<T *>::clear();
    }

private:
    bool autoDelete;
};

#endif
