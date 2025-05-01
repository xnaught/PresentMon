import { createRouter, createWebHistory } from 'vue-router'
import MainView from '@/views/MainView.vue'
import HomeView from '@/views/HomeView.vue'
import LoadoutConfigView from '@/views/LoadoutConfigView.vue'
import GraphConfigView from '@/views/GraphConfigView.vue'

const router = createRouter({
  history: createWebHistory(import.meta.env.BASE_URL),
  routes: [
    {
      path: '/',
      redirect: { name: 'main' },
    },
    {
      path: '/main',
      name: 'main',
      component: MainView,
    },
    {
      path: '/graphs/:index',
      name: 'graph-config',
      component: GraphConfigView,
      props: true,
    },
    {
      path: '/readouts/:index',
      name: 'readout-config',
      component: HomeView,
      props: true,
    },
    {
      path: '/overlay',
      name: 'overlay-config',
      component: HomeView,
    },
    {
      path: '/metrics',
      name: 'metric-processing',
      component: HomeView,
    },
    {
      path: '/loadout',
      name: 'loadout-config',
      component: LoadoutConfigView,
    },
    {
      path: '/capture',
      name: 'capture-config',
      component: HomeView,
    },
    {
      path: '/other',
      name: 'other-config',
      component: HomeView,
    },
    {
      path: '/flash',
      name: 'flash-config',
      component: HomeView,
    },
  ],
})

export default router
