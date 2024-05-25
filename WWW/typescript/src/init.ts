import { createPreview } from "./elements/preview";
import { createToolbar } from "./elements/toolbar";
import { createSaveLoad } from "./elements/saveLoad";
import { render } from "./render";
import { EditImage, window } from "./types";
import { createRectInput } from "./elements/newRect";


export function initApp() {
    const appRoot = document.getElementById('app') ?? document.body;
    appRoot.classList.add('container', 'mx-auto', 'p-4');
    const preview = createPreview();
    appRoot.appendChild(preview);
    const toolbar = createToolbar();
    appRoot.appendChild(toolbar);
    const newRect = createRectInput();
    appRoot.appendChild(newRect);
    const saveLoad = createSaveLoad();
    appRoot.appendChild(saveLoad);

    window.editImage = new EditImage(500, 500)
    window.currentColor = '#ff0000'
    render()
}