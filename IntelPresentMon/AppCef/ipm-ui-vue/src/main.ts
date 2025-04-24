import '@/assets/global.css'

import { createApp } from 'vue'
import { createPinia } from 'pinia'

// Vuetify
import { createVuetify } from 'vuetify'
import 'vuetify/styles'
// css
import '@mdi/font/css/materialdesignicons.css'
import '@fontsource/roboto/index.css'

import App from '@/App.vue'
import router from '@/router'
import { isDevBuild } from './core/env-vars'
import { md2 } from 'vuetify/blueprints'

if (isDevBuild()) {
    const script = document.createElement('script');
    script.src = 'http://localhost:8098';
    document.head.appendChild(script);
}

const vuetify = createVuetify({
    blueprint: md2,
    theme: {
        defaultTheme: 'customDark',
        themes: {
            customDark: {
                dark: true,
                colors: {
                    primary: '#1976D2',   // Vuetify 2 blue
                    secondary: '#424242', // Vuetify 2 grey
                    accent: '#82B1FF',
                    error: '#FF5252',
                    info: '#2196F3',
                    success: '#4CAF50',
                    warning: '#FFC107',
                    background: '#121212',
                    surface: '#1E1E1E',
                },
            },
        },
    },
})

const app = createApp(App)

app.use(createPinia())
app.use(router)
app.use(vuetify)

app.mount('#app')
