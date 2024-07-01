***

Do kompilacji `zad2.ts` użyłem [rollup.js](https://rollupjs.org/guide/en/). 

Aby skonfigurować środowisko, utworzyłem plik `package.json`:

```json
{
  "name": "zad2",
  "version": "1.0.0",
  "description": "",
  "main": "index.js",
  "scripts": {
    "build": "rollup -c"
  },
  "keywords": [],
  "type": "module",
  "author": "",
  "license": "ISC",
  "devDependencies": {
    "@rollup/plugin-terser": "^0.4.4",
    "@rollup/plugin-typescript": "^11.1.6",
    "rollup": "^4.17.2",
    "tslib": "^2.6.2",
    "typescript": "^5.4.5"
  }
}
```

Następnie zainstalowałem wymagane pakiety:

```bash
pnpm install
```

Użyłem pnpm, ponieważ jest to szybsza alternatywa dla npm i yarn.

Następnie utworzyłem plik konfiguracyjny `rollup.config.js`:

```js
import typescript from '@rollup/plugin-typescript';
import terser from '@rollup/plugin-terser';


export default {
  input: 'zad2.ts',
  output: {
    file: 'zad2.js',
    format: 'cjs'
  },
  plugins: [
    typescript({
      compilerOptions: {
        strict: true
      }
    }),
    terser(),
    {
      renderChunk(code) {
        return code.replace(/"use strict";/, '/** *** **/\n"use strict";');
      }
    }
  ]
};
```

Ostatecznie, aby skompilować plik `zad2.ts` do `zad2.js`, użyłem komendy:

```bash
pnpm run build
```