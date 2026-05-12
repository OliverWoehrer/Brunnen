// Import Global Assets:
import '@/assets/index.css';

// Initialize MDUI Components:
import 'mdui/mdui.css';
import { setColorScheme } from 'mdui/functions/setColorScheme.js';
setColorScheme("#8BC34A");

// Import Page Views:
import DefaultLayout from '@/layouts/DefaultLayout.vue';
import FullscreenLayout from '@/layouts/FullscreenLayout.vue';
import DashboardView from '@/views/DashboardView.vue'
import DataView from '@/views/DataView.vue'
import ErrorView from '@/views/ErrorView.vue'
import ProfileView from '@/views/ProfileView.vue'
import SettingsView from '@/views/SettingsView.vue'

// Define Replace Function:
function replaceOnError(code, message) {
    const params = new URLSearchParams({ code: code, message: message })
    window.location.replace(`/error.html`)
    // window.location.replace(`/error?${params.toString()}`)
}

// Initialize Vue Router:
import { createRouter, createWebHistory } from 'vue-router';
const router = createRouter({
    history: createWebHistory(),
    routes: [
        {
            path: "/", // path prefix
            component: DefaultLayout,
            children: [
                { path: "/", redirect: "/dashboard" },
                { path: "/dashboard", component: DashboardView },
                { path: "/data", component: DataView },
                { path: "/settings", component: SettingsView },
                { path: "/profile", component: ProfileView },
                { path: "/:pathMatch(.*)", redirect: to => ({path: "/error", query: {code:404, message:"Page not found"}})  }, // error on unknown path
                
            ]
        },
        {
            path: "/error",
            component: FullscreenLayout,
            children: [
                { path: "", component: ErrorView, props: route => ({ code: route.query.code, message: route.query.message }) },
            ]
        },
    ]
});

// Initialize Vue App:
import { createApp } from 'vue'
import App from './App.vue'
const app = createApp(App);
app.use(router);
app.mount('#app');
