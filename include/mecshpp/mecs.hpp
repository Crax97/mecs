#pragma once

#include "mecs/base.h"
#include "mecs/iterator.h"
#include "mecs/mecs.h"
#include "mecs/registry.h"
#include "mecs/world.h"

#include <compare>
#include <print>
#include <sstream>
#include <tuple>
#include <utility>

#define MECS_CONSTRUCTORS(Type)            \
    Type() = default;                      \
    Type(const Type&) = delete;            \
    Type& operator=(const Type&) = delete; \
                                           \
    Type(Type&& rhs) noexcept              \
        : mHandle(rhs.mHandle)             \
    {                                      \
        rhs.mHandle = nullptr;             \
    }                                      \
    Type& operator=(Type&& rhs) noexcept   \
    {                                      \
        mHandle = rhs.mHandle;             \
        rhs.mHandle = nullptr;             \
        return *this;                      \
    }

#define DEFINE_ID(Struct)                                                                          \
    namespace mecs {                                                                               \
    struct Struct {                                                                                \
        MecsU32 mID { MECS_INVALID };                                                              \
        constexpr auto operator<=>(const Struct& other) const = default;                           \
        constexpr bool operator==(const Struct& other) const = default;                            \
        constexpr static Struct invalid() { return { MECS_INVALID }; }                             \
        constexpr bool isValid() const { return mID != MECS_INVALID; }                             \
        constexpr bool isNull() const { return mID == MECS_INVALID; }                              \
        constexpr MecsU32 id() const { return mID; }                                               \
    };                                                                                             \
    }                                                                                              \
    namespace std {                                                                                \
    template <>                                                                                    \
    struct hash<mecs::Struct> {                                                                    \
        size_t operator()(const mecs::Struct& mID) const { return std::hash<MecsU32>()(mID.mID); } \
    };                                                                                             \
    template <>                                                                                    \
    struct formatter<mecs::Struct, char> {                                                         \
        constexpr auto parse(format_parse_context& ctx)                                            \
        {                                                                                          \
            return std::formatter<MecsU32>().parse(ctx);                                           \
        }                                                                                          \
        constexpr auto format(mecs::Struct& strukt, format_context& ctx) const                     \
        {                                                                                          \
            std::ostringstream out;                                                                \
            out << #Struct << " {" << strukt.id() << "}";                                          \
            return std::ranges::copy(std::move(out).str(), ctx.out()).out;                         \
        }                                                                                          \
    };                                                                                             \
    }

DEFINE_ID(ComponentID);
DEFINE_ID(PrefabID);
DEFINE_ID(EntityID);

namespace mecs {

namespace detail {

    template <typename T>
    static void init(void* ptr)
    {
        new (ptr) T();
    }

    template <typename T>
    static void copy(const void* src, void* dest, [[maybe_unused]] MecsSize size)
    {
        T* tDest = reinterpret_cast<T*>(dest);
        const T* tSrc = reinterpret_cast<const T*>(src);
        *tDest = *tSrc;
    }

    template <typename T>
    static void destroy(void* ptr)
    {
        T* tPtr = reinterpret_cast<T*>(ptr);
        tPtr->~T();
    }

    template <typename T>
    struct ParameterInfo {
        using RawType = T::DONT_USE_RAW_TYPES_USE_REFERENCES_OR_POINTERS;
    };

    template <typename T>
    struct ParameterInfo<T*> {
        using RawType = T;
        using Pointer = T*;
        constexpr static bool kIsConst = false;
    };

    template <typename T>
    struct ParameterInfo<const T*> {
        using RawType = T;
        using Pointer = const T*;
        constexpr static bool kIsConst = true;
    };

    template <typename T>
    struct ParameterInfo<T&> {
        using RawType = T;
        using Pointer = T*;
        constexpr static bool kIsConst = false;
    };

    template <typename T>
    struct ParameterInfo<const T&> {
        using RawType = T;
        using Pointer = const T*;
        constexpr static bool kIsConst = true;
    };
}

template <typename T>
struct RegistrationInfo {
    inline static ComponentID mComponentID;

    static void init(MecsRegistry* reg, const char* name)
    {
        static ComponentInfo gComponentInfo {
            .name = name,
            .size = sizeof(T),
            .align = alignof(T),
            .init = mecs::detail::init<T>,
            .copy = mecs::detail::copy<T>,
            .destroy = mecs::detail::destroy<T>,
        };
        mComponentID = { mecsRegistryAddRegistration(reg, &gComponentInfo) };
    }

    static ComponentID getComponentID()
    {
        MECS_ASSERT(mComponentID != ComponentID::invalid() && "Component was not registered");
        return mComponentID;
    }
};

namespace detail {
    template <typename... Args, std::size_t... I>
    void initIterator(MecsIterator* iterator, std::index_sequence<I...> idx)
    {
        (mecsIterComponent(iterator, RegistrationInfo<typename detail::ParameterInfo<Args>::RawType>::getComponentID().id(), I), ...);
    }

    template <typename... Args, std::size_t... I>
    std::tuple<Args...> getIteratorArguments(MecsIterator* iterator, std::index_sequence<I...> idx)
    {
        return { *static_cast<typename detail::ParameterInfo<Args>::Pointer>(mecsIteratorGetArgument(iterator, I))... };
    }
}

class PrefabBuilder {
public:
    template <typename T, typename... Args>
    PrefabBuilder& withComponent(Args&&... args)
    {
        T defaultVal = T(std::forward<Args>(args)...);
        mecsRegistryPrefabAddComponentWithDefaults(mHandleistry, mPrefabID, RegistrationInfo<T>::getComponentID().id(), &defaultVal);
        return *this;
    }

    operator PrefabID() const
    {
        return { mPrefabID };
    }

private:
    friend class Registry;
    PrefabBuilder(MecsRegistry* reg, MecsPrefabID prefab)
        : mHandleistry(reg)
        , mPrefabID(prefab)
    {
    }
    MecsRegistry* mHandleistry;
    MecsPrefabID mPrefabID;
};

class EntityBuilder {
public:
    template <typename T, typename... Args>
    EntityBuilder& setComponent(Args&&... args)
    {
        T val = T(std::forward<Args>(args)...);
        T* ptr = reinterpret_cast<T*>(mecsWorldEntityGetComponent(mWorld, mEntityID, RegistrationInfo<T>::getComponentID().id()));
        *ptr = std::move(val);
        return *this;
    }

    template <typename T, typename... Args>
    EntityBuilder& withComponent(Args&&... args)
    {
        T defaultVal = T(std::forward<Args>(args)...);
        T* ptr = reinterpret_cast<T*>(mecsWorldAddComponent(mWorld, mEntityID, RegistrationInfo<T>::getComponentID().id()));
        *ptr = std::move(defaultVal);
        return *this;
    }
    operator EntityID() const
    {
        return { mEntityID };
    }

private:
    friend class World;
    EntityBuilder(MecsWorld* world, MecsEntityID entityID)
        : mWorld(world)
        , mEntityID(entityID)
    {
    }
    MecsWorld* mWorld;
    MecsEntityID mEntityID;
};

class MECS_API Registry {
public:
    Registry(const MecsRegistryCreateInfo& registryInfo = {});
    ~Registry();

    MECS_CONSTRUCTORS(Registry)

    ComponentID addRegistration(const ComponentInfo& componentInfo);

    PrefabBuilder createPrefab();
    void addPrefabComponent(PrefabID prefab, ComponentID component, const void* defaultValue = nullptr);
    void* getPrefabComponent(PrefabID prefab, ComponentID component);
    void removePrefabComponent(PrefabID prefab, ComponentID component);
    void destroyPrefab(PrefabID prefab);

    template <typename T>
    ComponentID addRegistration(const char* name)
    {
        RegistrationInfo<T>::init(mHandle, name);
        return RegistrationInfo<T>::getComponentID();
    }

    template <typename T>
    void addPrefabComponent(PrefabID prefab, const T& defaultValue)
    {
        addPrefabComponent(prefab, RegistrationInfo<T>::getComponentID(), static_cast<const void*>(&defaultValue));
    }
    template <typename T>
    T* getPrefabComponent(PrefabID prefab)
    {
        return static_cast<T*>(getPrefabComponent(prefab, RegistrationInfo<T>::getComponentID()));
    }
    template <typename T>
    T* removePrefabComponent(PrefabID prefab)
    {
        return removePrefabComponent(prefab, RegistrationInfo<T>::getComponentID());
    }

    [[nodiscard]]
    MecsRegistry* getHandle() const
    {
        return mHandle;
    }

private:
    MecsRegistry* mHandle { nullptr };
};
template <typename... Args>
class Iterator;

class MECS_API World {
public:
    MECS_CONSTRUCTORS(World)
    World(Registry& registry, const MecsWorldCreateInfo& worldCreateInfo = {});
    ~World();

    EntityBuilder spawnEntity(const MecsEntityInfo& entityInfo = {});
    EntityBuilder spawnEntityPrefab(PrefabID prefab, const MecsEntityInfo& entityInfo = {});
    void entityAddComponent(EntityID entity, ComponentID component);
    [[nodiscard]]
    bool entityHasComponent(EntityID entity, ComponentID component) const;
    [[nodiscard]]
    void* entityGetComponent(EntityID entity, ComponentID component) const;
    void entityRemoveComponent(EntityID entity, ComponentID component);
    void destroyEntity(EntityID entity);
    void flushEvents();

    template <typename T>
    [[nodiscard]]
    bool entityHasComponent(EntityID entity) const
    {
        return entityHasComponent(entity, RegistrationInfo<T>::getComponentID());
    }

    template <typename T>
    [[nodiscard]]
    T* entityGetComponent(EntityID entity) const
    {
        return reinterpret_cast<T*>(entityGetComponent(entity, RegistrationInfo<T>::getComponentID()));
    }

    template <typename T>
    void entityAddComponent(EntityID entity)
    {
        entityAddComponent(entity, RegistrationInfo<T>::getComponentID());
    }

    template <typename T>
    void entityRemoveComponent(EntityID entity)
    {
        return entityRemoveComponent(entity, RegistrationInfo<T>::getComponentID());
    }

    template <typename... Args>
    Iterator<Args...> acquireIterator()
    {
        return Iterator<Args...>(*this);
    };

    [[nodiscard]]
    MecsWorld* getHandle() const
    {
        return mHandle;
    }

private:
    template <typename... Args>
    friend class Iterator;
    World(MecsWorld* world)
        : mHandle(world)
    {
    }
    MecsWorld* mHandle { nullptr };
};

template <typename... Args>
class Iterator {
public:
    ~Iterator()
    {
        release();
    }

    void begin()
    {
        mecsIteratorBegin(mHandle);
    }
    [[nodiscard]]
    bool advance()
    {
        return mecsIteratorAdvance(mHandle);
    }

    void release()
    {

        if (mHandle == nullptr) { return; }
        mecsWorldReleaseIterator(mecsIteratorGetWorld(mHandle), mHandle);
        mHandle = nullptr;
    }

    EntityID getEntityID()
    {
        return { mecsIteratorGetEntity(mHandle) };
    }
    World getWorld()
    {
        return World { mecsIteratorGetWorld(mHandle) };
    }

    std::tuple<Args...> get()
    {
        return detail::getIteratorArguments<Args...>(mHandle, std::make_index_sequence<sizeof...(Args)>());
    }

    [[nodiscard]]
    MecsIterator* getHandle() const
    {
        return mHandle;
    }

private:
    friend class World;

    Iterator(World& world)
    {
        mHandle = mecsWorldAcquireIterator(world.getHandle());
        detail::initIterator<Args...>(mHandle, std::make_index_sequence<sizeof...(Args)>());
        mecsIteratorFinalize(mHandle);
    }
    MecsIterator* mHandle { nullptr };
};

namespace utils {
    template <typename... Args>
    MecsSize count(Iterator<Args...>& iterator)
    {
        return mecsUtilIteratorCount(iterator.getHandle());
    }
}

}
