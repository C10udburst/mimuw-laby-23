import { render } from "../render";
import { window } from "../types";
import { load, save } from "../ws";

export function createSaveLoad() {
    const buttons = document.createElement('div')
    buttons.classList.add('flex', 'space-x-4', 'mt-4');
    buttons.appendChild(createSaveBtn());
    buttons.appendChild(createLoadBtn());
    buttons.appendChild(createSubscribeBtn());
    return buttons
}

function createSaveBtn() {
    const saveBtn = document.createElement('button');
    saveBtn.textContent = 'Save';
    saveBtn.classList.add('btn', 'btn-success');
    saveBtn.addEventListener('click', () => {
        save(window.editImage?.serialize() || null);
    });
    return saveBtn;
}

function createLoadBtn() {
    const loadBtn = document.createElement('button');
    loadBtn.textContent = 'Load';
    loadBtn.classList.add('btn', 'btn-primary');
    loadBtn.addEventListener('click', () => {
        load();
    });
    return loadBtn;
}

function createSubscribeBtn() {
    const subscribeBtn = document.createElement('button');
    subscribeBtn.textContent = 'Subscribe';
    subscribeBtn.classList.add('btn', 'btn-primary');
    subscribeBtn.addEventListener('click', () => {
        let ws = new WebSocket(`ws${location.protocol === 'https:' ? 's' : ''}://${location.host}/ws2`);
        ws.onmessage = ev => {
            window.editImage?.deserialize(ev.data);
            render();
        }
        ws.onopen = () => ws.send(`subscribe`);
    });
    return subscribeBtn;
}