
```{raw} html
<div style="text-align: center; margin-top: 2rem;">
  <img src="_static/mqss_logo.svg" class="logo-light" style="width: 20%;">
  <img src="_static/mqss_logo_dark.svg" class="logo-dark" style="width: 20%;">
</div>
<style>
  body[data-theme="light"] .logo-dark  { display: none; }
  body[data-theme="dark"]  .logo-light { display: none; }
  body[data-theme="auto"] .logo-dark   { display: none; }
  @media (prefers-color-scheme: dark) {
    body[data-theme="auto"] .logo-dark  { display: inline; }
    body[data-theme="auto"] .logo-light { display: none; }
  }
</style>
```

# Munich Quantum Software Stack (MQSS) Quantum Compilation Suite

<!-- Include the content of README.md between the pair of markers DOXYGEN MAIN. -->

Welcome to the MQSS-Quantum Compilation Suite documentation page!
This documentation provides helpful information to get you started with the Quantum
Compilation suite of the Munich Quantum Software Stack (MQSS).

```{toctree}
:maxdepth: 2
:caption: Contents

README
guide
passes
transpiler
templates
faq
```
