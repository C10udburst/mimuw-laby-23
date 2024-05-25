import { render } from "../render";
import { Rectangle, window } from "../types";

export function createRectInput() {
    const form = document.createElement('form');
    form.classList.add('space-y-4', 'w-full', 'flex', 'flex-row', 'items-center');
    const xInput = document.createElement('input');
    xInput.type = 'number';
    xInput.placeholder = 'x';
    xInput.classList.add('input');
    const yInput = document.createElement('input');
    yInput.type = 'number';
    yInput.placeholder = 'y';
    yInput.classList.add('input');
    const widthInput = document.createElement('input');
    widthInput.type = 'number';
    widthInput.placeholder = 'width';
    widthInput.classList.add('input');
    const heightInput = document.createElement('input');
    heightInput.type = 'number';
    heightInput.placeholder = 'height';
    heightInput.classList.add('input');
    const colorInput = document.createElement('input');
    colorInput.type = 'color';
    colorInput.classList.add('input');
    const submit = document.createElement('button');
    submit.type = 'submit';
    submit.textContent = 'Add Rectangle';
    submit.classList.add('btn', 'btn-primary');
    form.appendChild(xInput);
    form.appendChild(yInput);
    form.appendChild(widthInput);
    form.appendChild(heightInput);
    form.appendChild(colorInput);
    form.appendChild(submit);
    form.addEventListener('submit', ev => {
        ev.preventDefault();
        const x = parseInt(xInput.value);
        const y = parseInt(yInput.value);
        const width = parseInt(widthInput.value);
        const height = parseInt(heightInput.value);
        const color = colorInput.value;
        window.editImage?.rectangles.push(new Rectangle(x, y, width, height, color));
        render();
    });
    return form;
}