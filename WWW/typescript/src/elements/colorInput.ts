import { window } from '../types';

export function createColorInput() {
    const colorInput = document.createElement('input');
    colorInput.type = 'color';
    colorInput.value = '#ff0000';
    colorInput.classList.add('button', 'input');
    colorInput.addEventListener('input', ev => {
        const target = ev.target as HTMLInputElement;
        window.currentColor = target.value;
    });
    return colorInput;
}