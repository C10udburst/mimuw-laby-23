import { EditImage } from "./types";

export function render() {
    const preview = document.getElementById('preview') as SVGElement | null
    if (!preview) return;

    // @ts-ignore
    const image = window.editImage as EditImage | null
    if (!image) return;

    image.setDOM(preview)
}