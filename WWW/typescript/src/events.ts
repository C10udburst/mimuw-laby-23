import { render } from "./render";
import { EditImage, Rectangle } from "./types";


export function onCanvasDown(mouseX: number, mouseY: number) {
    // @ts-ignore
    window.startCoords = [mouseX, mouseY]
}

export function onCanvasUp(mouseX: number,  mouseY: number) {
    // @ts-ignore
    const [prevX, prevY] = window.startCoords as number[]

    // @ts-ignore
    const image = window.editImage as EditImage | null
    if (!image) return;

    const x = Math.min(mouseX, prevX)
    const y = Math.min(mouseY, prevY)
    const width  = Math.max(mouseX, prevX) - x
    const height = Math.max(mouseY, prevY) - y

    image.rectangles.push(new Rectangle(
        x, y, width, height,
        "red"
    ))

    render()
}