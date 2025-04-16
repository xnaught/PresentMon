import { createRouter, createWebHistory } from 'vue-router'
import HomeView from '../views/HomeView.vue'

const router = createRouter({
  history: createWebHistory(import.meta.env.BASE_URL),
  routes: [
    {
      path: '/',
      redirect: { name: 'simple' },
    },
    {
      path: '/graphs/:index',
      name: 'graph-config',
      component: HomeView,
      props: true,
    },
    {
      path: '/readouts/:index',
      name: 'readout-config',
      component: HomeView,
      props: true,
    },
    {
      path: '/simple',
      name: 'simple',
      component: HomeView,
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
      component: HomeView,
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
