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
}

export class EditImage {
    width: number
    height: number
    rectangles: Rectangle[]

    constructor(w: number, h: number) {
        this.height = h;
        this.width = w;
        this.rectangles = []
    }

    setDOM(element: SVGElement) {
        element.innerHTML = ''
        element.setAttribute('viewBox', `0 0 ${this.width} ${this.height}`)
        element.style.width = `${this.width}px`
        element.style.height = `${this.height}px`
        for (const rect of this.rectangles) {
            element.appendChild(rect.toDOMElement())
        }
    }
}