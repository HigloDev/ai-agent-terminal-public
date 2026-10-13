'use strict';

const skinNames = {default: '默认', eva: 'EVA / NERV', kon: 'K-ON!', pokemon: 'Pokémon'};
const scenes = {
  reading: {label: '阅读进展', kicker: '把正文放在中心', title: ['读得下，', '也跟得上。'], description: '打开原会话，阅读最新回复与历史内容。左杆慢滚、右杆快滚，代码与长文本都有自己的阅读空间。', keys: [['左杆', '慢滚'], ['右杆', '快滚']]},
  draft: {label: '核对草稿', kicker: '先核对，再发送', title: ['一句话，', '也认真确认。'], description: '按住 Y 录音，松手后进入文字核对。草稿保留目标会话，确认内容后再发送；识别与桌面联动仍需真机验收。', keys: [['Y', '录音'], ['A', '确认发送']]},
  artifact: {label: '查看成果', kicker: '从回复走向成果', title: ['看清变化，', '再决定下一步。'], description: '从会话进入成果文件，阅读源端提供的正文与 diff。保留原会话与成果来源，让下一步有据可依。', keys: [['A', '预览'], ['X', '查看变更']]}
};
let currentSkin = 'default';
let currentScene = 'reading';
const heroImage = document.querySelector('#hero-screen');
const sceneImage = document.querySelector('#scene-screen');
const skinButtons = [...document.querySelectorAll('[data-skin]')];
const sceneTabs = [...document.querySelectorAll('[data-scene]')];
const mobileLayout = window.matchMedia('(max-width: 700px)');
function updateTabOrientation() {document.querySelector('.scene-tabs').setAttribute('aria-orientation', mobileLayout.matches ? 'horizontal' : 'vertical');}
updateTabOrientation();
mobileLayout.addEventListener('change', updateTabOrientation);

function updateImages() {
  const theme = skinNames[currentSkin];
  heroImage.src = `./site-assets/${currentSkin}-reading.webp`;
  heroImage.alt = `${theme}皮肤：原生掌机程序的会话阅读界面，合成对话`;
  heroImage.parentElement.setAttribute('aria-label', `放大查看${theme}皮肤阅读截图`);
  sceneImage.src = `./site-assets/${currentSkin}-${currentScene}.webp`;
  sceneImage.alt = `${theme}皮肤${scenes[currentScene].label}场景，实际程序渲染，合成数据`;
  sceneImage.parentElement.setAttribute('aria-label', `放大查看${theme}皮肤${scenes[currentScene].label}截图`);
  document.querySelector('#scene-theme').textContent = theme;
}
function selectSkin(skin) {
  currentSkin = skin;
  skinButtons.forEach(button => {const active = button.dataset.skin === skin; button.setAttribute('aria-pressed', String(active)); button.classList.toggle('active', active);});
  updateImages();
  document.querySelector('#skin-status').textContent = `已切换到${skinNames[skin]}皮肤。`;
}
skinButtons.forEach((button, index) => {
  button.addEventListener('click', () => selectSkin(button.dataset.skin));
  button.addEventListener('keydown', event => {
    if (!['ArrowLeft', 'ArrowRight', 'Home', 'End'].includes(event.key)) return;
    event.preventDefault();
    const next = event.key === 'Home' ? 0 : event.key === 'End' ? skinButtons.length - 1 : (index + (event.key === 'ArrowRight' ? 1 : -1) + skinButtons.length) % skinButtons.length;
    skinButtons[next].focus(); selectSkin(skinButtons[next].dataset.skin);
  });
});
function selectScene(scene) {
  currentScene = scene;
  const content = scenes[scene];
  sceneTabs.forEach(button => {const active = button.dataset.scene === scene; button.setAttribute('aria-selected', String(active)); button.tabIndex = active ? 0 : -1;});
  document.querySelector('#scene-panel').setAttribute('aria-labelledby', `tab-${scene}`);
  document.querySelector('#scene-kicker').textContent = content.kicker;
  const heading = document.querySelector('#scene-title');
  heading.replaceChildren(document.createTextNode(content.title[0]), document.createElement('br'), document.createTextNode(content.title[1]));
  document.querySelector('#scene-description').textContent = content.description;
  const keys = document.querySelector('#scene-keys');
  keys.replaceChildren();
  content.keys.forEach(([key, label]) => {const kbd = document.createElement('kbd'); kbd.textContent = key; const span = document.createElement('span'); span.textContent = label; keys.append(kbd, span);});
  updateImages();
}
sceneTabs.forEach((button, index) => {
  button.addEventListener('click', () => selectScene(button.dataset.scene));
  button.addEventListener('keydown', event => {
    if (!['ArrowLeft', 'ArrowRight', 'ArrowUp', 'ArrowDown', 'Home', 'End'].includes(event.key)) return;
    event.preventDefault();
    const next = event.key === 'Home' ? 0 : event.key === 'End' ? sceneTabs.length - 1 : (index + (['ArrowDown', 'ArrowRight'].includes(event.key) ? 1 : -1) + sceneTabs.length) % sceneTabs.length;
    sceneTabs[next].focus(); selectScene(sceneTabs[next].dataset.scene);
  });
});

const menuButton = document.querySelector('.menu-toggle');
const navigation = document.querySelector('#navigation');
function closeMenu() {menuButton.setAttribute('aria-expanded', 'false'); navigation.classList.remove('open');}
menuButton.addEventListener('click', () => {const open = menuButton.getAttribute('aria-expanded') !== 'true'; menuButton.setAttribute('aria-expanded', String(open)); navigation.classList.toggle('open', open);});
navigation.addEventListener('click', event => {if (event.target.closest('a')) closeMenu();});
document.addEventListener('keydown', event => {if (event.key === 'Escape' && navigation.classList.contains('open')) {closeMenu(); menuButton.focus();}});
document.addEventListener('click', event => {if (!event.target.closest('.header')) closeMenu();});

const dialog = document.querySelector('#image-dialog');
const dialogImage = document.querySelector('#dialog-image');
let imageOpener;
document.querySelectorAll('.image-open').forEach(button => button.addEventListener('click', () => {
  const image = button.classList.contains('device-screen') ? heroImage : sceneImage;
  dialogImage.src = image.src; dialogImage.alt = image.alt;
  document.querySelector('#dialog-title').textContent = `${skinNames[currentSkin]} / ${button.classList.contains('device-screen') ? '阅读进展' : scenes[currentScene].label}`;
  imageOpener = button; dialog.showModal(); dialog.scrollTo(0, 0);
}));
document.querySelector('#dialog-close').addEventListener('click', () => dialog.close());
dialog.addEventListener('click', event => {const rect = dialog.getBoundingClientRect(); if (event.target === dialog && (event.clientX < rect.left || event.clientX > rect.right || event.clientY < rect.top || event.clientY > rect.bottom)) dialog.close();});
dialog.addEventListener('close', () => imageOpener?.focus({preventScroll: true}));

document.querySelector('#copy-code').addEventListener('click', async () => {
  const code = document.querySelector('#setup-code');
  const status = document.querySelector('#copy-status');
  try {await navigator.clipboard.writeText(code.textContent); status.textContent = '命令已复制。请先阅读开发配置；这些命令仅验证电脑端逻辑。';}
  catch {const range = document.createRange(); range.selectNodeContents(code); const selection = window.getSelection(); selection.removeAllRanges(); selection.addRange(range); status.textContent = '已选中命令，请使用系统复制操作。';}
});
