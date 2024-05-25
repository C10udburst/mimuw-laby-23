import { toRelative } from '../utils';
import { onCanvasDown, onCanvasUp } from '../events';
import { window } from '../types';

export function createPreview() {
    const container = document.createElement('div');
    const figure = document.createElement('figure');
    figure.classList.add('w-fit', 'h-fit');
    const svg = document.createElementNS('http://www.w3.org/2000/svg', 'svg');
    window.preview = svg;
    svg.id = 'preview';
    svg.classList.add('w-full', 'h-full');
    figure.appendChild(svg);
    container.appendChild(figure);
    container.classList.add('border', 'border-gray-300', 'rounded-md', 'overflow-hidden', 'w-fit', 'h-fit');
    container.addEventListener('mousedown', ev => toRelative(ev, onCanvasDown))
    container.addEventListener('mouseup', ev => {
        if (toRelative(ev, onCanvasUp)) {
            ev.target?.dispatchEvent(new Event('click'));
        }
    });
    return container;
}