import { window } from "../types";

export function createDeleteBtn() {
    const deleteBtn = document.createElement('button');
    deleteBtn.textContent = 'Delete';
    deleteBtn.classList.add('btn', 'btn-error');
    deleteBtn.addEventListener('click', () => {
        window.editImage?.deleteSelected();
    });
    deleteBtn.style.display = 'none';
    window.deleteBtn = deleteBtn;
    return deleteBtn;
}