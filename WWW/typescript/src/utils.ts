export function toRelative(event: MouseEvent, cb: (x: number, y: number) => void) {
    const preview = document.getElementById('preview')
    if (!preview) return;
    const rect = preview.getBoundingClientRect()
    cb(
        event.clientX - rect.x,
        event.clientY - rect.y
    )
}