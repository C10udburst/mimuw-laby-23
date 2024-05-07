import typescript from '@rollup/plugin-typescript';

export default {
  input: 'src/index.ts',
  output: {
    dir: 'public',
    format: 'cjs'
  },
  plugins: [typescript({
    compilerOptions: {
      strict: true
    }
  })]
};