import { onCanvasDown, onCanvasUp } from "./events";
import { render } from "./render";
import { EditImage } from "./types";
import { toRelative } from "./utils";

function createPreview() {
    const container = document.createElement('div');
    const figure = document.createElement('figure');
    const svg = document.createElementNS('http://www.w3.org/2000/svg', 'svg');
    // @ts-ignore TS2239
    window.preview = svg;
    svg.id = 'preview';
    svg.classList.add('w-full', 'h-full');
    figure.appendChild(svg);
    container.appendChild(figure);
    container.classList.add('border', 'border-gray-300', 'rounded-md', 'overflow-hidden', 'w-auto')
    container.addEventListener('mousedown', ev => toRelative(ev, onCanvasDown))
    container.addEventListener('mouseup', ev => toRelative(ev, onCanvasUp))
    return container;
}


export function initApp() {
    const appRoot = document.getElementById('app') ?? document.body;
    const preview = createPreview();
    appRoot.appendChild(preview);
    appRoot.classList.add('container', 'mx-auto', 'p-4');

    // @ts-ignore
    window.editImage = new EditImage(500, 500)
    render()
}