import { render } from "./render"

export class Rectangle {
    x: number
    y: number
    width: number
    height: number
    color: string

    constructor(x: number, y: number, width: number, height: number, color: string) {
        this.x = x;
        this.y = y;
        this.width = width;
        this.height = height;
        this.color = color;
    }

    toDOMElement(): SVGRectElement {
        const rect = document.createElementNS('http://www.w3.org/2000/svg', 'rect')
        rect.setAttribute('x', this.x.toString())
        rect.setAttribute('y', this.y.toString())
        rect.setAttribute('width', this.width.toString())
        rect.setAttribute('height', this.height.toString())
        rect.setAttribute('fill', this.color)
        return rect
    }

    serialize(): string {
        return JSON.stringify({
            x: this.x,
            y: this.y,
            width: this.width,
            height: this.height,
            color: this.color
        })
    }

    toName(): string {
        return `${this.width}x${this.height} ${this.color}`
    }
}

export function deserializeRectangle(serialized: string): Rectangle {
    const data = JSON.parse(serialized)
    return new Rectangle(data.x, data.y, data.width, data.height, data.color)
}

export class EditImage {
    width: number
    height: number
    selectedRect: number | null
    rectangles: Rectangle[]

    constructor(w: number, h: number) {
        this.height = h;
        this.width = w;
        this.rectangles = []
        this.selectedRect = null
    }

    setDOM(element: SVGElement|null) {
        if (!element) return;
        element.innerHTML = ''
        element.setAttribute('viewBox', `0 0 ${this.width} ${this.height}`)
        element.style.width = `${this.width}px`
        element.style.height = `${this.height}px`
        for (const i in this.rectangles) {
            const rectEl = this.rectangles[i].toDOMElement()
            rectEl.addEventListener('click', () => {
                this.selectRect(parseInt(i))
            })
            element.appendChild(rectEl)
        }
    }

    serialize(): string {
        return JSON.stringify({
            width: this.width,
            height: this.height,
            rectangles: this.rectangles.map(r => r.serialize())
        })
    }

    deserialize(serialized: string) {
        try {
            const data = JSON.parse(serialized)
            this.width = data.width
            this.height = data.height
            this.rectangles = data.rectangles.map(deserializeRectangle)
            this.selectedRect = null
        } catch (e) {
            alert('Failed to load image: ' + serialized)
        }
    }

    selectRect(i: number) {
        this.selectedRect = i;
        window.deleteBtn?.style.removeProperty('display');
        // @ts-ignore
        window.deleteBtn?.textContent = `Delete ${this.rectangles[i].toName()}`;
    }

    deleteSelected() {
        if (this.selectedRect !== null) {
            this.rectangles.splice(this.selectedRect, 1)
            this.selectedRect = null
            // @ts-ignore
            window.deleteBtn?.style.display = 'none';
            render()
        }
    }
}

export interface CustomWindow extends Window {
    preview: SVGElement | null;
    deleteBtn: HTMLButtonElement | null;
    editImage: EditImage | null;
    startCoords: [number, number] | null;
    currentColor: string;
}

export let window: CustomWindow;