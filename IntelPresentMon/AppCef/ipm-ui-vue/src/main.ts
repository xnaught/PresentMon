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

if (isDevBuild()) {
    const script = document.createElement('script');
    script.src = 'http://localhost:8098';
    document.head.appendChild(script);
}

const vuetify = createVuetify({
    theme: { defaultTheme: 'dark' },
})

const app = createApp(App)

app.use(createPinia())
app.use(router)
app.use(vuetify)

app.mount('#app')
