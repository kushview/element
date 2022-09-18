
#include <element/plugin.h>
#include <memory>
#include <vector>

/** Feature implementation template. 
 *  The template parameter should be a
*/
template <typename CType>
class FeatureData
{
public:
    using data_type = CType;
    ~FeatureData() = default;
    const data_type* get() const noexcept { return &data; }

protected:
    FeatureData() = default;
    data_type& reference() { return data; }

private:
    data_type data;
};

class FeatureType
{
public:
    virtual ~FeatureType() = default;
    const elFeature* c_type() const noexcept { return &f; }
    const std::string& ID() const noexcept { return fid; }
    const void* data() const noexcept { return f.data; }

protected:
    FeatureType() = default;

    void set_details (const char* ID, void* data)
    {
        fid = std::string (ID);
        f.ID = fid.c_str();
        f.data = data;
    }

private:
    std::string fid;
    elFeature f;
};

/** List of referenced features. */
class Features final
{
public:
    using VectorType = std::vector<const elFeature*>;

    Features() { features.push_back (nullptr); }
    Features (const elFeature* const* cfeatures)
    {
        for (int i = 0; cfeatures[i] != nullptr; ++i)
        {
            features.push_back (cfeatures[i]);
        }
        features.push_back (nullptr);
    }

    ~Features() { features.clear(); }

    void clear() noexcept { features.clear(); }
    size_t size() const noexcept { return features.size() - 1; }
    void reserve (size_t num) { features.reserve (num); }
    auto begin() const noexcept { return features.cbegin(); }
    auto end() const noexcept { return std::prev (features.cend()); }

    const void* find (const char* ID) const noexcept
    {
        for (const auto* f : *this)
            if (strcmp (f->ID, ID) == 0)
                return f->data;
        return nullptr;
    }

    bool contains (const char* ID) const noexcept { return nullptr != find (ID); }
    elFeatures c_type() const noexcept { return features.data(); }
    operator const elFeature* const*() const noexcept
    {
        return features.data();
    }

private:
    VectorType features;
    Features (const Features& o) = delete;
    Features (const Features&& o) = delete;
    Features& operator= (const Features& o) = delete;
};

/** Collection of feature implementations. */
class FeatureStore
{
public:
    FeatureStore() = default;
    virtual ~FeatureStore() = default;

    operator elFeatures() const noexcept
    {
        build_cached (false);
        return cached.data();
    }

    /** Add a new feature to the list.
        
        The passed object will be owned and deleted by the Features
        class.

        @param ft The feature to add
     */
    void add_type (FeatureType* ft) noexcept
    {
        auto sft = std::shared_ptr<FeatureType> (ft);
        types.push_back (sft);
        ++dirty;
    }

    const void* data (const std::string& feature) const noexcept
    {
        for (const auto& f : types)
            if (f->ID() == feature)
                return f->data();
        return nullptr;
    }

    void clear()
    {
        while (types.size() > 0)
        {
            auto ptr = types.back();
            types.pop_back();
            ptr.reset();
        }
    }

private:
    EL_DISABLE_COPY (FeatureStore)
    EL_DISABLE_MOVE (FeatureStore)

    using TypeVec = std::vector<std::shared_ptr<FeatureType>>;
    TypeVec types;

    Features::VectorType cached;
    uint32_t dirty = 1;

    void clear_cached()
    {
        cached.clear();
        cached.reserve (types.size() + 1);
        ++dirty;
    }

    void build_cached (bool force) const
    {
        (const_cast<FeatureStore*> (this))->build_cached_impl (force);
    }

    void build_cached_impl (bool force)
    {
        if (dirty == 0 && ! force)
            return;
        clear_cached();
        for (const auto& t : types)
            cached.push_back (t->c_type());
        cached.push_back (nullptr);
        dirty = 0;
    }
};
