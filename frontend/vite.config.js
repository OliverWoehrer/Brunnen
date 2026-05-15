import { fileURLToPath, URL } from 'node:url'
import { defineConfig } from 'vite'
import path from 'path';
import vue from '@vitejs/plugin-vue'
import vueDevTools from 'vite-plugin-vue-devtools'

// https://vite.dev/config/
export default defineConfig({
    appType: 'spa',
    root: path.resolve(__dirname), // ensure root is the frontend folder
    build: {
        outDir: path.resolve(__dirname, 'dist'), // compile to 'dist' folder
        emptyOutDir: true, // clean output directory before every build
        rollupOptions: {
            input: {
                spa: 'index.html',
                login: 'login.html',
            }
        }
    },
    plugins: [
        vue({
            template: {
                compilerOptions: {
                    isCustomElement: (tag) => tag.startsWith('mdui-') || tag.includes('-'), // exclude tags starting with "mdui-"
                },
            },
        }),
        vueDevTools(),
    ],
    resolve: {
        alias: {
            '@': fileURLToPath(new URL('./src', import.meta.url))
        },
    },
})
