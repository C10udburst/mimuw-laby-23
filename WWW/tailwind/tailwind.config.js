module.exports = {
  content: ["./src/**/*.{html,js}"],
  theme: {
    extend: {
      gridTemplateAreas: {
        'layout': [
          'header header',
          'nav    main',
          'nav    footer',
        ],
      },
      gridTemplateColumns: {
        'layout': '18rem 1fr',
      },
      gridTemplateRows: {
        'layout': '6rem 1fr auto',
      },
    },
  },
  plugins: [
    require('@savvywombat/tailwindcss-grid-areas')
  ]
}