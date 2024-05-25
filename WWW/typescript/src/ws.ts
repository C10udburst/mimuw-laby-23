import { render } from './render';
import { window } from './types';

export function load() {
    let key = prompt('Enter name of saved image');
    if (!key) return;
    let ws = new WebSocket(`ws${location.protocol === 'https:' ? 's' : ''}://${location.host}/ws`);
    ws.onmessage = ev => {
        window.editImage?.deserialize(ev.data);
        render();
    }
    ws.onopen = () => ws.send(`load ${key}`);
}

export function save(data: string|null) {
    if (!data) return;
    let key = prompt('Enter name for saving');
    if (!key) return;
    let ws = new WebSocket(`ws${location.protocol === 'https:' ? 's' : ''}://${location.host}/ws`);
    ws.onopen = () => ws.send(`save ${key} ${data}`);
}
