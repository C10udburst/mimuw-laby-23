<script setup>

import { ref } from 'vue'

const imageData = ref(false)
const show = ref(false)

const bus = new BroadcastChannel('imageSelected')

bus.onmessage = (event) => {
    try {
        show.value = true
        const ws = new WebSocket('ws://localhost:8765')
        ws.onmessage = (event) => {
            try {
                imageData.value = JSON.parse(event.data)
            } catch (e) {
                dialogOnClose()
                console.error(e)
            }
        }
        ws.onopen = () => {
            ws.send(event.data)
        }
    } catch (e) {
        dialogOnClose()
        console.error(e)
    }
}


function dialogOnClose() {
    show.value = false
    imageData.value = false
}

</script>

<template>
    <v-dialog v-model="show" max-width="800" @update:modelValue="dialogOnClose">
        <v-card v-if="imageData" :width="imageData.w" :height="imageData.h">
            <svg xmlns="http://www.w3.org/2000/svg" :width="imageData.w" :height="imageData.h">
                <rect v-for="r in imageData.rects" :x="r.x" :y="r.y" :width="r.w" :height="r.h" :fill="r.color" />
            </svg>
        </v-card>
        <v-card v-else>
            <v-card-title>
                Loading...
            </v-card-title>
            <v-card-text>
                <v-progress-linear indeterminate />
            </v-card-text>
        </v-card>
    </v-dialog>
</template>

