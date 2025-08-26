#pragma once

#include "mecs/base.h"
#include "mecs/iterator.h"
#include "mecs/mecs.h"
#include "mecs/registry.h"
#include "mecs/world.h"
#include "mecshpp/mecsrtti.hpp"

#include <compare>
#include <print>
#include <sstream>
#include <tuple>
#include <type_traits>
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
        auto format(mecs::Struct& strukt, format_context& ctx) const                               \
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

template <typename T>
struct With { };
template <typename T>
struct Not { };

template <typename T>
    requires(HasRTTI<T>)
struct RegistrationInfo {
    inline static mecs::ComponentID mComponentId {};
    static mecs::ComponentID init(MecsRegistry* reg)
    {
        const ComponentInfo& componentInfo = rttiOf<T>::componentInfo();
        mComponentId = { mecsRegistryAddRegistration(reg, &componentInfo) };
        return mComponentId;
    }
    static ComponentID getComponentID()
    {
        MECS_ASSERT(mComponentId.isValid());
        return mComponentId;
    }
};
namespace detail {

    template <typename T>
    struct ParameterInfo {
        using RawType = T::DONT_USE_RAW_TYPES_USE_REFERENCES_OR_POINTERS;
    };
    template <>
    struct ParameterInfo<mecs::EntityID> {
        using RawType = mecs::EntityID;
        using Pointer = mecs::EntityID*;
        constexpr static bool kIsConst = true;
        static void addArgument(MecsIterator* iterator, MecsSize argIndex)
        {
        }
        static RawType getArgument(MecsIterator* iterator, MecsSize argIndex)
        {
            return { mecsIteratorGetEntity(iterator) };
        }
    };

    template <typename T>
    struct ParameterInfo<T*> {
        using RawType = T;
        using Pointer = T*;
        constexpr static bool kIsConst = false;
        static void addArgument(MecsIterator* iterator, MecsSize argIndex)
        {
            mecsIterComponentFilter(iterator, RegistrationInfo<RawType>::getComponentID().id(), MecsIteratorFilter::Access, argIndex);
        }
        static RawType& getArgument(MecsIterator* iterator, MecsSize argIndex)
        {
            return *reinterpret_cast<RawType*>(mecsIteratorGetArgument(iterator, argIndex));
        }
    };

    template <typename T>
    struct ParameterInfo<const T*> {
        using RawType = T;
        using Pointer = const T*;
        constexpr static bool kIsConst = true;
        static void addArgument(MecsIterator* iterator, MecsSize argIndex)
        {
            mecsIterComponentFilter(iterator, RegistrationInfo<RawType>::getComponentID().id(), MecsIteratorFilter::Access, argIndex);
        }
        static RawType& getArgument(MecsIterator* iterator, MecsSize argIndex)
        {
            return *reinterpret_cast<RawType*>(mecsIteratorGetArgument(iterator, argIndex));
        }
    };

    template <typename T>
    struct ParameterInfo<T&> {
        using RawType = T;
        using Pointer = T*;
        constexpr static bool kIsConst = false;
        static void addArgument(MecsIterator* iterator, MecsSize argIndex)
        {
            mecsIterComponentFilter(iterator, RegistrationInfo<RawType>::getComponentID().id(), MecsIteratorFilter::Access, argIndex);
        }
        static RawType& getArgument(MecsIterator* iterator, MecsSize argIndex)
        {
            return *reinterpret_cast<RawType*>(mecsIteratorGetArgument(iterator, argIndex));
        }
    };

    template <typename T>
    struct ParameterInfo<const T&> {
        using RawType = T;
        using Pointer = const T*;
        constexpr static bool kIsConst = true;
        static void addArgument(MecsIterator* iterator, MecsSize argIndex)
        {
            mecsIterComponentFilter(iterator, RegistrationInfo<RawType>::getComponentID().id(), MecsIteratorFilter::Access, argIndex);
        }
        static RawType& getArgument(MecsIterator* iterator, MecsSize argIndex)
        {
            return *reinterpret_cast<RawType*>(mecsIteratorGetArgument(iterator, argIndex));
        }
    };

    template <typename T>
    struct ParameterInfo<With<T>> {
        using RawType = std::remove_reference_t<std::remove_const_t<T>>;
        using Pointer = nullptr_t;
        constexpr static bool kIsConst = true;
        static void addArgument(MecsIterator* iterator, MecsSize argIndex)
        {
            mecsIterComponentFilter(iterator, RegistrationInfo<RawType>::getComponentID().id(), MecsIteratorFilter::With, argIndex);
        }

        static With<T> getArgument(MecsIterator* iterator, MecsSize argIndex)
        {
            return {};
        }
    };
    template <typename T>
    struct ParameterInfo<Not<T>> {
        using RawType = std::remove_reference_t<std::remove_const_t<T>>;
        using Pointer = nullptr_t;
        constexpr static bool kIsConst = true;
        static void addArgument(MecsIterator* iterator, MecsSize argIndex)
        {
            mecsIterComponentFilter(iterator, RegistrationInfo<RawType>::getComponentID().id(), MecsIteratorFilter::Not, argIndex);
        }

        static Not<T> getArgument(MecsIterator* iterator, MecsSize argIndex)
        {
            return {};
        }
    };
}

namespace detail {
    template <typename... Args, std::size_t... I>
    void initIterator(MecsIterator* iterator, std::index_sequence<I...> idx)
    {
        (detail::ParameterInfo<Args>::addArgument(iterator, I), ...);
    }

    template <typename... Args, std::size_t... I>
    std::tuple<Args...> getIteratorArguments(MecsIterator* iterator, std::index_sequence<I...> idx)
    {
        return { detail::ParameterInfo<Args>::getArgument(iterator, I)... };
    }

    template <typename... Args, std::size_t... I, typename Func>
    void callFuncHelper(const std::tuple<Args...>& values, Func&& func, std::index_sequence<I...> idx)
    {
        func(std::get<I>(values)...);
    }
}

class PrefabBuilder {
public:
    template <typename T, typename... Args>
    PrefabBuilder& withComponent(Args&&... args)
    {
        T defaultVal = T(std::forward<Args>(args)...);
        mecsRegistryPrefabAddComponentWithDefaults(mHandle, mPrefabID, RegistrationInfo<T>::getComponentID().id(), &defaultVal);
        return *this;
    }

    operator PrefabID() const
    {
        return { mPrefabID };
    }

private:
    friend class Registry;
    PrefabBuilder(MecsRegistry* reg, MecsPrefabID prefab)
        : mHandle(reg)
        , mPrefabID(prefab)
    {
    }
    MecsRegistry* mHandle;
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
    [[nodiscard]]
    MecsSize getPrefabNumComponents(PrefabID prefab) const;
    [[nodiscard]]
    ComponentID getPrefabComponentIDByIndex(PrefabID prefab, MecsSize componentIndex) const;

    [[nodiscard]]
    MecsSize getNumPrefabs() const;

    [[nodiscard]]
    MecsSize getNumComponents() const;
    [[nodiscard]]
    mecs::ComponentID getComponentIDByName(const std::string& name) const;
    [[nodiscard]]
    mecs::ComponentID getComponentIDByIndex(MecsSize index) const;
    [[nodiscard]]
    const ComponentInfo& getComponentInfoByIndex(MecsSize index) const;
    [[nodiscard]]
    const ComponentInfo& getComponentInfoByComponentID(mecs::ComponentID componentID) const;

    template <typename T>
        requires(HasRTTI<T>)
    ComponentID addRegistration()
    {
        return RegistrationInfo<T>::init(mHandle);
    }

    template <typename F>
    void forEachRegisteredComponent(F&& func)
    {
        const MecsSize numComponents = getNumComponents();
        for (MecsSize i = 0; i < numComponents; i++) {
            mecs::ComponentID componentID = getComponentIDByIndex(i);
            const ComponentInfo& info = getComponentInfoByIndex(i);
            func(componentID, info);
        }
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
    T& entityGetComponent(EntityID entity) const
    {
        return *reinterpret_cast<T*>(entityGetComponent(entity, RegistrationInfo<T>::getComponentID()));
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
    using TupleType = std::tuple<Args...>;
    MECS_CONSTRUCTORS(Iterator)
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

    template <typename Func>
    void forEach(Func&& func)
    {
        begin();
        while (advance()) {
            auto value = get();
            detail::callFuncHelper(value, func, std::make_index_sequence<sizeof...(Args)>());
        }
    }

    TupleType first()
    {
        begin();
        bool firstFound = advance();
        MECS_ASSERT(firstFound && "At least one entity must match when calling first()");
        return get();
    }
    TupleType get()
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
