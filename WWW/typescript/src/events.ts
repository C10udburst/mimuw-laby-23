import { render } from "./render";
import { EditImage, Rectangle, window } from "./types";


export function onCanvasDown(mouseX: number, mouseY: number) {
    window.startCoords = [mouseX, mouseY]
}

export function onCanvasUp(mouseX: number,  mouseY: number) : boolean {
    const [prevX, prevY] = window.startCoords as number[]

    const x = Math.min(mouseX, prevX)
    const y = Math.min(mouseY, prevY)
    const width  = Math.max(mouseX, prevX) - x
    const height = Math.max(mouseY, prevY) - y

    if (width < 5 || height < 5) {
        window.startCoords = null
        return true
    }

    window.editImage?.rectangles.push(new Rectangle(
        x, y, width, height,
        window.currentColor
    ))

    render()

    window.startCoords = null

    return false
}