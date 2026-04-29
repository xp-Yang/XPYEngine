#ifndef World_hpp
#define World_hpp

#if ENABLE_ECS

#include <bitset>
#include <vector>

#include <assert.h>

#include "Base/Signal/Signal.hpp"

namespace Meta
{
    namespace Register
    {
        void allMetaRegister();
    }
}

namespace ecs
{

    const int MAX_ENTITIES = 512;
    const int MAX_COMPONENTS = 32;
    typedef std::bitset<MAX_COMPONENTS> ComponentMask;

    // 每个Entity保存一份bitset，大小是MAX_COMPONENTS。标记了该Entity有没有某种类型的Component数据
    // World保存Entities数组
    class Entity
    {
    public:
        Entity() = delete;
        Entity(int id) { m_id = id; }
        Entity(const Entity &rhs)
        {
            m_id = rhs.getId();
            m_mask = rhs.getMask();
        }
        bool operator==(const Entity &rhs) const
        {
            return getId() == rhs.getId();
        }
        int getId() const { return m_id; }
        ComponentMask &getMask() { return m_mask; }
        const ComponentMask &getMask() const { return m_mask; }

    private:
        friend void Meta::Registration::allMetaRegister();

        int m_id;
        ComponentMask m_mask;
    };

    // 每个Pool保存相同类型的Component数组，数组大小是MAX_ENTITIES个。
    // World保存Pools数组
    class ComponentPool
    {
    public:
        ComponentPool(size_t componentTypeSize)
        {
            m_componentTypeSize = componentTypeSize;
            m_data = new char[m_componentTypeSize * MAX_ENTITIES];
        }

        ~ComponentPool()
        {
            delete[] m_data;
        }

        inline void *get(size_t entity_id)
        {
            return m_data + entity_id * m_componentTypeSize;
        }

        char *m_data{nullptr};
        size_t m_componentTypeSize{0};
    };

    // todo 更改实现，有bug：get的不一定是已创建
    // 为每个Pool分配id。
    extern int g_componentCounter;
    template <class T>
    int getComponentPoolId()
    {
        static int componentPoolId = g_componentCounter++;
        return componentPoolId;
    }

    class CameraComponent;
    class DirectionalLightComponent;
    class SkyboxComponent;
    template <typename... ComponentTypes>
    class EnttView;
    class World
    {
    public:
        static World &get()
        {
            static World instance;
            return instance;
        }

        // 所有entity都从这里创建
        Entity create_entity()
        {
            static int entt_id = 0;
            m_entities.emplace_back(entt_id++);
            return m_entities.back();
        }

        void destroy_entity(const Entity &entity);

        template <typename T>
        bool hasComponent(const Entity &entity)
        {
            int entt_id = entity.getId();
            int pool_id = getComponentPoolId<T>();
            if (!m_entities[entt_id].getMask().test(pool_id))
                return false;
            return true;
        }

        template <typename T>
        T *getComponent(const Entity &entity)
        {
            int entt_id = entity.getId();

            int pool_id = getComponentPoolId<T>();
            if (!m_entities[entt_id].getMask().test(pool_id))
                return nullptr;

            T *component = static_cast<T *>(m_component_pools[pool_id]->get(entt_id));
            return component;
        }

        template <typename T>
        T &addComponent(const Entity &entity)
        {
            int entt_id = entity.getId();

            int pool_id = getComponentPoolId<T>();
            m_entities[entt_id].getMask().set(pool_id);

            if (pool_id >= m_component_pools.size())
            {
                m_component_pools.push_back(new ComponentPool(sizeof(T)));
                assert(false);
            }

            // Looks up the component in the pool, and initializes it with placement new
            T *component = new (m_component_pools[pool_id]->get(entt_id)) T();
            emit componentInserted(entt_id, pool_id);
            return *component;
        }

        template <typename... ComponentTypes>
        void removeComponent(const Entity &entity)
        {
            int poolIds[] = {getComponentPoolId<ComponentTypes>()...};
            for (int i = 0; i < sizeof...(ComponentTypes); i++)
            {
                // 池中都没这个component
                if (poolIds[i] >= m_component_pools.size())
                {
                    assert(false);
                }
            }
            // free
            int entt_id = entity.getId();
            (void)std::initializer_list{(destroyComponent<ComponentTypes>(entt_id), 0)...};
        }

        template <typename... ComponentTypes>
        EnttView<ComponentTypes...> entityView() { return EnttView<ComponentTypes...>(); }

        CameraComponent *getMainCameraComponent();
        DirectionalLightComponent *getMainDirectionalLightComponent();
        SkyboxComponent *getSkyboxComponent();
        std::vector<Entity> getPickedEntities();

    signals:
        Signal<int, int> componentInserted;
        Signal<int, int> componentRemoved;

    protected:
        World();
        World(const World &) = delete;
        ~World();

        template <typename T>
        void init_component_pools_()
        {
            int pool_id = getComponentPoolId<T>();
            assert(pool_id == m_component_pools.size());
            m_component_pools.push_back(new ComponentPool(sizeof(T)));
        }

        template <typename... ComponentTypes>
        void init_component_pools()
        {
            (void)std::initializer_list{(init_component_pools_<ComponentTypes>(), 0)...};
        }

        template <typename T>
        void destroyComponent(int entt_id)
        {
            int pool_id = getComponentPoolId<T>();
            T *component = reinterpret_cast<T *>(m_component_pools[pool_id]->get(entt_id));
            if (m_entities[entt_id].getMask().test(pool_id))
            {
                // todo delete component;
                m_entities[entt_id].getMask().reset(pool_id);
                emit componentRemoved(entt_id, pool_id);
            }
        }

    private:
        friend void Meta::Registration::allMetaRegister();
        template <typename... ComponentTypes>
        friend class EnttView;

        std::vector<Entity> m_entities;
        std::vector<ComponentPool *> m_component_pools;
    };

    template <typename... ComponentTypes>
    class EnttView
    {
    public:
        EnttView()
            : world(&World::get())
        {
            if (sizeof...(ComponentTypes) == 0)
                view_all = true;
            else
            {
                // Unpack the template parameters into an initializer list
                int poolIds[] = {getComponentPoolId<ComponentTypes>()...};
                for (int i = 0; i < sizeof...(ComponentTypes); i++)
                    view_mask.set(poolIds[i]);
                view_all = false;
            }
        }

        class Iter
        {
        public:
            Iter(const EnttView *entt_view, int entity_idx)
                : entt_view_ref(entt_view), entity_idx(entity_idx) {}

            Entity operator*()
            {
                return entt_view_ref->world->m_entities[entity_idx];
            }

            Iter &operator++()
            {
                entity_idx++;
                for (; entity_idx < entt_view_ref->world->m_entities.size(); entity_idx++)
                {
                    if (entt_view_ref->view_all || (entt_view_ref->view_mask == (entt_view_ref->world->m_entities[entity_idx].getMask() & entt_view_ref->view_mask)))
                        break;
                }
                return *this;
            }

            Iter operator++(int)
            {
                Iter tmp = *this;
                ++*this;
                return tmp;
            }

            bool operator==(const Iter &another) const
            {
                return entity_idx == another.entity_idx;
            }

            bool operator!=(const Iter &another) const
            {
                return entity_idx != another.entity_idx;
            }

            int entity_idx;
            const EnttView *entt_view_ref{nullptr};
        };

        const Iter begin() const
        {
            int i = 0;
            const auto &all_entities = world->m_entities;
            for (i = 0; i < all_entities.size(); i++)
            {
                if (view_all || ((all_entities[i].getMask() & view_mask) == view_mask))
                    break;
            }
            // 如果没找到
            if (i == all_entities.size())
            {
                return end();
            }
            return Iter(this, i);
        }

        const Iter end() const
        {
            // 配合operator++()
            return Iter(this, (int)world->m_entities.size());
        }

    private:
        World *world{nullptr};
        ComponentMask view_mask;
        bool view_all{false};

        friend class Iter;
    };

}

#endif

#endif // ENABLE_ECS