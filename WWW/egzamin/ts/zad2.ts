// ***

type WsNotification = {
    method: string,
    value: number
};

class Handler {
    current_value: number = 0;
    visualizer: HTMLParagraphElement;
    reload_btn: HTMLButtonElement;
    
    constructor(visualizer: HTMLParagraphElement, reload_btn: HTMLButtonElement) {
        this.current_value = 0;
        this.visualizer = visualizer;
        this.reload_btn = reload_btn;
    }

    update() {
        this.visualizer.textContent = this.current_value.toString();
    }

    set_value(value: number) {
        this.current_value = value;
        this.update();
    }

    divide_value(divisor: number) {
        this.current_value /= divisor;
        this.update();
    }

    multiply_value(multiplier: number) {
        this.current_value *= multiplier;
        this.update();
    }

    handle_notification(notification: WsNotification) {
        try {
            switch (notification.method) {
                case 'divide':
                    this.divide_value(notification.value);
                    break;
                case 'multiply':
                    this.multiply_value(notification.value);
                    break;
                default:
                    throw new Error('Unknown method');
            }
        } catch (e) {
            if (e instanceof Error) {
                this.set_error(e);
            } else {
                this.set_error(new Error('Unknown error'));
            }
        }
    }

    set_error(e: Error) {
        console.error(e);
        this.visualizer.style.color = 'red';
        this.visualizer.textContent = `Error: ${e.message}`;
        this.reload_btn.style.visibility = 'visible';
    }
    
}

function setup_ws(handler: Handler) {
    const ws = new WebSocket(`ws${window.location.protocol === 'https:' ? 's' : ''}://${window.location.host}/ws`);

    ws.onopen = () => {
        console.log('WebSocket connection established');
    };

    ws.onmessage = (event) => {
        try {
            const data = JSON.parse(event.data) as WsNotification;
            handler.handle_notification(data);
        } catch (e) {
            if (e instanceof Error) {
                handler.set_error(e);
            } else {
                handler.set_error(new Error('Unknown error'));
            }
            ws.close();
        }
    };

    ws.onclose = () => {
        handler.set_error(new Error('WebSocket connection closed'));
    };

    ws.onerror = (e) => {
        if (e instanceof Event) {
            handler.set_error(new Error('WebSocket error'));
        } else {
            handler.set_error(e);
        }
    };
}

function reloadConn(handler: Handler) {
    handler.visualizer.style.color = 'black';
    handler.reload_btn.style.visibility = 'hidden';
    fetch('/current')
        .then(response => response.json())
        .catch(error => handler.set_error(error))
        .then(data => {
                handler.set_value(data.current);
                setup_ws(handler);
            });
}



function onLoad() {
    const p_el = document.querySelector('p') as HTMLParagraphElement;
    const reload_btn = document.body.appendChild(document.createElement('button'));
    reload_btn.textContent = 'Reload';
    reload_btn.onclick = () => reloadConn(handler);
    reload_btn.style.visibility = 'hidden';
    let handler = new Handler(p_el, reload_btn);

    reloadConn(handler);
}

document.addEventListener('DOMContentLoaded', onLoad);