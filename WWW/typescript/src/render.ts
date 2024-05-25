import { window } from "./types";

export function render() {
    window.editImage?.setDOM(window.preview)
}