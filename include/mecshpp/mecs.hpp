#pragma once

#include "mecs/base.h"
#include "mecs/iterator.h"
#include "mecs/mecs.h"
#include "mecs/registry.h"
#include "mecs/world.h"
#include "mecshpp/base.hpp"
#include "mecshpp/mecsrtti.hpp"

#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>

namespace mecs {
namespace detail {
    struct InvokeHelper;
}


template<typename... Args>
class Iterator;

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
        const auto& rtti = rttiOf<T>();
        const ComponentInfo componentInfo {
            .typeID = rtti.typeID,
            .name = rtti.name,
            .size = rtti.size,
            .align = rtti.align,
            .init = rtti.init,
            .copy = rtti.copy,
            .move = rtti.move,
            .destroy = rtti.destroy,
            .setup = rtti.setup,
            .teardown = rtti.teardown,
        };
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
        constexpr static bool kIsComponent = false;
        constexpr static MecsIteratorFilter kFilterType = MecsIteratorFilter::Access;
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
        constexpr static bool kIsComponent = true;
        constexpr static MecsIteratorFilter kFilterType = MecsIteratorFilter::Access;
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
        constexpr static bool kIsComponent = true;
        constexpr static MecsIteratorFilter kFilterType = MecsIteratorFilter::Access;
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
        constexpr static bool kIsComponent = true;
        constexpr static MecsIteratorFilter kFilterType = MecsIteratorFilter::Access;
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
        constexpr static bool kIsComponent = true;
        constexpr static MecsIteratorFilter kFilterType = MecsIteratorFilter::Access;
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
        using Pointer = std::nullptr_t;
        constexpr static bool kIsConst = true;
        constexpr static bool kIsComponent = true;
        constexpr static MecsIteratorFilter kFilterType = MecsIteratorFilter::With;
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
        using Pointer = std::nullptr_t;
        constexpr static bool kIsConst = true;
        constexpr static bool kIsComponent = true;
        constexpr static MecsIteratorFilter kFilterType = MecsIteratorFilter::Not;
        static void addArgument(MecsIterator* iterator, MecsSize argIndex)
        {
            mecsIterComponentFilter(iterator, RegistrationInfo<RawType>::getComponentID().id(), MecsIteratorFilter::Not, argIndex);
        }

        static Not<T> getArgument(MecsIterator* iterator, MecsSize argIndex)
        {
            return {};
        }
    };

    template<typename... Args>
    constexpr MecsSize countComponents()
    {
        return ((ParameterInfo<Args>::kIsComponent ? 1U : 0U) +...);
    }
    template<MecsSize S>
    void addComponentIDs(std::array<MecsComponentID, S>&, size_t)
    {

    }

    template<MecsSize S, typename A, typename... Rest>
    void addComponentIDs(std::array<MecsComponentID, S>& outArray, size_t idx)
    {

        if constexpr (ParameterInfo<A>::kIsComponent) {
            outArray[idx] = RegistrationInfo<typename ParameterInfo<A>::RawType>::getComponentID().id();
            addComponentIDs<S, Rest...>(outArray, idx + 1);
        } else {
            addComponentIDs<S, Rest...>(outArray, idx);
        }
    }

    template<MecsSize S, typename A>
    void addComponentIDs(std::array<MecsComponentID, S>& outArray, size_t idx)
    {

        if constexpr (ParameterInfo<A>::kIsComponent) {
            outArray[idx] = RegistrationInfo<typename ParameterInfo<A>::RawType>::getComponentID();
        }
    }

    template<MecsSize S>
    void addFilter(std::array<MecsIteratorFilter, S>&, size_t)
    {

    }

    template<MecsSize S, typename A, typename... Rest>
    void addFilter(std::array<MecsIteratorFilter, S>& outArray, size_t idx)
    {

        if constexpr (ParameterInfo<A>::kIsComponent) {
            outArray[idx] = ParameterInfo<A>::kFilterType;
            addFilter<S, Rest...>(outArray, idx + 1);
        } else {
            addFilter<S, Rest...>(outArray, idx);
        }
    }

    template<MecsSize S, typename A>
    void addFilter(std::array<MecsIteratorFilter, S>& outArray, size_t idx)
    {
        if constexpr (ParameterInfo<A>::kIsComponent) {
            outArray[idx] = ParameterInfo<A>::kFilterType;
        }
    }

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
    struct IteratorHelper {
        template<typename... Args>
        static Iterator<Args...> makeIterator(MecsIterator* iter);
        template <class... Args>
        static void forget(Iterator<Args...>& iter);
    };

    template<typename S, typename Run, typename A, typename R>
    struct SystemBinderHelper;

    template<typename S = void>
    struct BasicSystemHelper {
        constexpr static bool kValue = false;
    };

    template<typename S, typename... Args>
    struct BasicSystemHelper<void(S::*)(mecs::World&, Iterator<Args...>&)> {
        constexpr static bool kValue = true;
    };

    template<typename S, typename... Args>
    struct SystemBinderHelper<
        S,
        void(S::*)(World&, Iterator<Args...>&),
        void(S::*)(World&, EntityID),
        void(S::*)(World&, EntityID)> {

        template<
            void(S::*systemRun)(World&, Iterator<Args...>&),
            void(S::*onEntityAdded)(World&, EntityID) = nullptr,
            void(S::*onEntityRemoved)(World&, EntityID) = nullptr>
        constexpr static MecsSystemID bind(MecsWorld* world, S* base)
        {
            constexpr MecsSize kNumComponents = detail::countComponents<Args...>();
            std::array<MecsComponentID, kNumComponents> componentIDs;
            std::array<MecsIteratorFilter, kNumComponents> filters;
            detail::addComponentIDs<kNumComponents, Args...>(componentIDs, 0);
            detail::addFilter<kNumComponents, Args...>(filters, 0);

            MecsDefineSystemInfo info;
            info.numComponents = kNumComponents;
            info.pComponents = componentIDs.data();
            info.pFilters = filters.data();
            info.systemData = reinterpret_cast<void*>(base);
            if constexpr(onEntityAdded != nullptr) {
                info.onEntityAdded = [](void* sysData, void* updateData, MecsEntityID entityID) {
                    S& sys = *static_cast<S*>(sysData);
                    World& world = *static_cast<World*>(updateData);
                    (sys.*onEntityAdded)(world, {entityID});
                };
            } else {
                info.onEntityAdded = nullptr;
            }
            if constexpr(onEntityRemoved != nullptr) {
                info.onEntityRemoved = [](void* sysData, void* updateData, MecsEntityID entityID) {
                    S& sys = *static_cast<S*>(sysData);
                    World& world = *static_cast<World*>(updateData);
                    (sys.*onEntityRemoved)(world, {entityID});
                };
            } else {
                info.onEntityRemoved = nullptr;
            }

            info.systemRun = [](void* sysData, void* updateData, MecsIterator* iterator) {
                S& sys = *static_cast<S*>(sysData);
                World& world = *static_cast<World*>(updateData);
                auto iter = detail::IteratorHelper::makeIterator<Args...>(iterator);
                (sys.*systemRun)(world, iter);
                detail::IteratorHelper::forget(iter);
            };

            info.systemFlags = 0;

            return mecsWorldDefineSystem(world, &info);
        }
    };

    template<typename S, typename... Args>
    struct SystemBinderHelper<
        S,
        void(S::*)(World&, Iterator<Args...>&),
        void*,
        void*> {

        template<
            void(S::*systemRun)(World&, Iterator<Args...>&)>
        constexpr static MecsSystemID bind(MecsWorld* world, S* base)
        {
            constexpr MecsSize kNumComponents = detail::countComponents<Args...>();
            std::array<MecsComponentID, kNumComponents> componentIDs;
            std::array<MecsIteratorFilter, kNumComponents> filters;
            detail::addComponentIDs<kNumComponents, Args...>(componentIDs, 0);
            detail::addFilter<kNumComponents, Args...>(filters, 0);

            MecsDefineSystemInfo info;
            info.numComponents = kNumComponents;
            info.pComponents = componentIDs.data();
            info.pFilters = filters.data();
            info.systemData = reinterpret_cast<void*>(base);
            info.onEntityAdded = nullptr;
            info.onEntityRemoved = nullptr;

            info.systemRun = [](void* sysData, void* updateData, MecsIterator* iterator) {
                S& sys = *static_cast<S*>(sysData);
                World& world = *static_cast<World*>(updateData);
                auto iter = detail::IteratorHelper::makeIterator<Args...>(iterator);
                (sys.*systemRun)(world, iter);
            };

            info.systemFlags = 0;

            return mecsWorldDefineSystem(world, &info);
        }
    };

    template<typename S, auto systemRun, auto onEntityAdded, auto onEntityRemoved>
    constexpr static MecsSystemID bindStaticSystem(MecsWorld* world, S* base)
    {
        return SystemBinderHelper<
            S,
            decltype(systemRun),
            decltype(onEntityAdded),
            decltype(onEntityRemoved)>::template bind<systemRun, onEntityAdded, onEntityRemoved>(world, base);
    }

    template<typename S, auto systemRun>
    constexpr static MecsSystemID bindStaticSystem(MecsWorld* world, S* base)
    {
        return SystemBinderHelper<
            S,
            decltype(systemRun), void*, void*>::template bind<systemRun>(world, base);
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
class MECS_API Any {
public:
    // NOLINTBEGIN
    enum Flags_ {
        Flags_IsPointer = 0x01
    };
    // NOLINTEND
    using DeleteFn = void (*)(void* data);
    template <typename T>
    static Any from(T value, MecsAllocator& alloc) noexcept
    {
        Any any;
        if constexpr (std::is_pointer_v<T>) {
            any.mData = reinterpret_cast<MecsU8*>(value);
            any.mFlags |= Flags_IsPointer;
            any.mDelete = []([[maybe_unused]]
                              void* ptr) { };
        } else {
            any.mData = reinterpret_cast<MecsU8*>(alloc.memAlloc(alloc.userData, sizeof(T), alignof(T)));
            new (any.mData) T { value };
            any.mDelete = [](void* ptr) {
                if constexpr (std::is_destructible_v<T>) {
                    T* tPtr = reinterpret_cast<T*>(ptr);
                    tPtr->~T();
                }
            };
        }
        any.mAllocator = alloc;
        return any;
    }

    Any(Any&& rhs) noexcept
    {
        if (&rhs == this) { return; }
        if (mData != nullptr) {
            mDelete(mData);
        }
        mData = rhs.mData;
        mDelete = rhs.mDelete;
        mFlags = rhs.mFlags;
        rhs.mData = nullptr;
        rhs.mDelete = nullptr;
    }

    Any& operator=(Any&& rhs) noexcept
    {
        if (&rhs == this) { return *this; }
        if (mData != nullptr) {
            mDelete(mData);
        }
        mData = rhs.mData;
        mDelete = rhs.mDelete;
        mFlags = rhs.mFlags;
        rhs.mData = nullptr;
        rhs.mDelete = nullptr;
        rhs.mFlags = 0;
        return *this;
    }

    ~Any() noexcept
    {
        if (mData == nullptr) { return; }
        mDelete(mData);

        if ((mFlags & Flags_IsPointer) == 0) {
            mAllocator.memFree(mAllocator.userData, mData);
        }
        mData = nullptr;
    }

    template <typename T>
    T cast() const
    {
        if constexpr(std::is_pointer_v<T>) {
            return reinterpret_cast<T>(mData);
        } else {
            return *reinterpret_cast<T*>(mData);
        }
    }

private:
    Any() noexcept = default;
    MecsU8* mData { nullptr };
    DeleteFn mDelete { nullptr };
    MecsAllocator mAllocator {};
    int mFlags { 0 };
};

class MECS_API ServiceRegistry {
public:
    template <typename T>
    void provide(T service, MecsAllocator& alloc)
    {
        SimpleID typeID = getID<T>();
        mServices.insert({ typeID, Any::from<T>(service, alloc) });
    }

    template <typename T>
    T get()
    {
        SimpleID typeID = getID<T>();
        MECS_ASSERT(mServices.contains(typeID) && "Service was not provided");
        return mServices.at(typeID).cast<T>();
    }

    template <typename T>
    const T& get() const
    {
        SimpleID typeID = getID<T>();
        MECS_ASSERT(mServices.contains(typeID) && "Service was not provided");
        return *mServices.at(typeID).cast<T>();
    }

private:
    using SimpleID = MecsU64;
    inline static SimpleID gAllIds = 0;
    template <typename T>
    static SimpleID getID()
    {
        static SimpleID gThisID = gAllIds++;
        return gThisID;
    }
    std::unordered_map<SimpleID, Any> mServices;
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
    void forEachRegisteredComponent(F&& func) const
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
template<typename S>
concept BasicSystem = requires(S) {
    {detail::BasicSystemHelper<decltype(&S::systemRun)>::kValue};
};

template<typename S>
concept HasEntityAdded = requires(S& system, World& world, EntityID entityID)
{
    {system.onEntityAdded(world, entityID)};
};

class MECS_API World {
public:
    MECS_CONSTRUCTORS(World)
    World(Registry& registry, const MecsWorldCreateInfo& worldCreateInfo = {});
    ~World();

    template<typename S, auto Run, auto Add, auto Remove>
    MecsSystemID addSystem(S* system)
    {
        return mecs::detail::bindStaticSystem<S, Run, Add, Remove>(mHandle, system);
    }

    template<typename S, auto Run>
    MecsSystemID addSystem(S* system)
    {
        return mecs::detail::bindStaticSystem<S, Run>(mHandle, system);
    }

    template<BasicSystem S>
    MecsSystemID addSystem(S* system)
    {
        if constexpr(HasEntityAdded<S>) {
            return mecs::detail::bindStaticSystem<S, &S::systemRun, &S::onEntityAdded, &S::onEntityRemoved>(mHandle, system);
        } else {
            return mecs::detail::bindStaticSystem<S, &S::systemRun>(mHandle, system);
        }
    }

    EntityBuilder spawnEntity(const MecsEntityInfo& entityInfo = {});
    EntityBuilder spawnEntityPrefab(PrefabID prefab, const MecsEntityInfo& entityInfo = {});
    EntityBuilder duplicateEntity(World& destinationWorld, EntityID sourceEntity);
    EntityBuilder duplicateEntity(EntityID sourceEntity) { return duplicateEntity(*this, sourceEntity); }
    void entityAddComponent(EntityID entity, ComponentID component);
    [[nodiscard]]
    bool entityHasComponent(EntityID entity, ComponentID component) const;
    [[nodiscard]]
    void* entityGetComponent(EntityID entity, ComponentID component) const;
    void entityRemoveComponent(EntityID entity, ComponentID component);
    void entityChanged(EntityID entity);
    [[nodiscard]]
    MecsSize entityGetNumComponents(EntityID entity) const;
    [[nodiscard]]
    ComponentID entityGetComponentByIndex(EntityID entity, MecsSize index) const;
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

    template <typename T>
    void registerService(T service)
    {
        MecsAllocator alloc = mecsWorldGetAllocator(mHandle);
        mServiceRegistry.provide(service, alloc);
    }

    template <typename T>
    T getService()
    {
        return mServiceRegistry.get<T>();
    }

private:
    template <typename... Args>
    friend class Iterator;
    friend struct detail::InvokeHelper;
    World(MecsWorld* world)
        : mHandle(world)
    {
    }
    MecsWorld* mHandle { nullptr };
    ServiceRegistry mServiceRegistry;
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
        [[maybe_unused]]
        bool firstFound
            = advance();
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
    friend class detail::IteratorHelper;

    Iterator(World& world)
    {
        mHandle = mecsWorldAcquireIterator(world.getHandle());
        detail::initIterator<Args...>(mHandle, std::make_index_sequence<sizeof...(Args)>());
        mecsIteratorFinalize(mHandle);
    }
    Iterator(MecsIterator* handle) noexcept
        : mHandle(handle) {}
    void forget()
    {
        mHandle = nullptr;
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

namespace detail {
    template <typename... Args>
    Iterator<Args...> IteratorHelper::makeIterator(MecsIterator* iter)
    {
        return {iter};
    }
    template <typename... Args>
    void IteratorHelper::forget(Iterator<Args...>& iter)
    {
        iter.forget();
    }
}
}
