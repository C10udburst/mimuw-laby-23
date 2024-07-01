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