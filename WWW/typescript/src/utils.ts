export function toRelative<Type>(event: MouseEvent, cb: (x: number, y: number) => Type): Type|null {
    const preview = document.getElementById('preview')
    if (!preview) return null;
    const rect = preview.getBoundingClientRect()
    return cb(
        event.clientX - rect.x,
        event.clientY - rect.y
    )
}