import { createColorInput } from "./colorInput";
import { createDeleteBtn } from "./deleteBtn";

export function createToolbar() {
    const toolbar = document.createElement('div');
    toolbar.classList.add('flex', 'space-x-4', 'mt-4');
    toolbar.appendChild(createColorInput());
    toolbar.appendChild(createDeleteBtn());
    return toolbar;
}