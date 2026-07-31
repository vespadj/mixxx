import {defineConfig} from "vite";
import {join} from "path";
import tailwindcss from "@tailwindcss/vite";

const __dirname = import.meta.dirname ;

export default defineConfig({
    // root: 'src' // does madness

    build: {
        manifest: true,
        emptyOutDir: true,
        // outDir: resolve(__dirname, "dist"),

        // Uncomment for debug
        // minify: false,
        sourcemap: true,

        rollupOptions: {
            input: {
                main: join(__dirname, "src/main.js"),
                styles: join(__dirname, "src/styles.css"),
            },
        },
    },

    plugins: [
        tailwindcss(), // v.4
    ],
});
