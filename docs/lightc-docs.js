// lightc-docs.js — vocabulary, styling hooks and navigation for the lightc
// API documentation. Listed in the :::use block of every page. The renderer
// provides window.tern before any add-on runs, and the CLI runs this file too,
// so it sees the same vocabulary the browser does. Behaviours are no-ops in the
// CLI.
(function () {
  'use strict';
  var tern = window.tern;
  if (!tern) return;

  // Callouts: coloured asides, keyed by name in docs.css.
  var callouts = {
    note: 'Note',
    tip: 'Tip',
    important: 'Important',
    warning: 'Warning',
    danger: 'Danger',
  };
  Object.keys(callouts).forEach(function (name) {
    tern.block(name, { tag: 'aside', class: 'callout', label: callouts[name] });
  });

  // A member entry: one function, macro or type, documented reference-style
  // (signature, then parameters and return value). Styled as a bordered card.
  tern.block('member', { tag: 'section', class: 'member' });

  // Numbered constructs, used where a figure or captioned table appears.
  tern.block('figure', { tag: 'figure', counter: 'figure', label: 'Figure', ref: '{label} {n}' });
  tern.block('table', { tag: 'table', counter: 'table', label: 'Table', ref: '{label} {n}' });

  // `::toc` lists this page's own headings.
  tern.leaf('toc', {
    tag: 'nav',
    transform: 'toc',
    depth: '2-3',
    label: 'On this page',
    attrs: { 'aria-label': 'Contents' },
  });

  // Top navigation bar: a <nav> leaf the CLI sees as a known name, filled in by
  // the behaviour below in the browser (and left empty under the CLI).
  var PAGES = [
    ['index.html', 'Home'],
    ['guide.html', 'Core'],
    ['heap.html', 'Heap'],
    ['threads.html', 'Threads'],
    ['coroutines.html', 'Coroutines'],
    ['async.html', 'Async & lightio'],
  ];
  tern.leaf('site-nav', { tag: 'nav', attrs: { 'aria-label': 'Documentation' } });
  tern.define('site-nav', function (el) {
    var here = location.pathname.split('/').pop() || 'index.html';
    el.textContent = '';
    var brand = document.createElement('a');
    brand.href = 'index.html';
    brand.className = 'brand';
    brand.textContent = 'lightc';
    el.appendChild(brand);
    PAGES.forEach(function (page) {
      var a = document.createElement('a');
      a.href = page[0];
      a.textContent = page[1];
      if (page[0] === here) a.setAttribute('aria-current', 'page');
      el.appendChild(a);
    });
  });
})();
