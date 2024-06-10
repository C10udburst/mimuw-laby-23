<!-- we will load image count from ws and display a list with pagination, 10 images per page -->

<script setup>
import { ref } from 'vue'

const tag = ref('')
const taglist = ref([])
const imageIds = ref([])
const imageCount = ref(0)
const page = ref(1)

function reload() {
    const ws = new WebSocket('ws://localhost:8765')
    ws.onmessage = (event) => {
        taglist.value = JSON.parse(event.data)
    }
    ws.onopen = () => {
        ws.send('tags')
    }
    setTag('')
}

function setTag(newTag) {
    tag.value = newTag
    const ws = new WebSocket('ws://localhost:8765')
    ws.onmessage = (event) => {
        imageIds.value = JSON.parse(event.data)
        imageCount.value = imageIds.value.length
    }
    ws.onopen = () => {
        ws.send(tag.value !== '' ? `tag:${tag.value}` : 'list')
    }
}


reload();

const bus = new BroadcastChannel('reload')
bus.onmessage = () => {
    reload()
}

const bus2 = new BroadcastChannel('imageSelected')
function viewImage(id) {
    bus2.postMessage(id)
}

</script>

<template>
    <v-select v-model="tag" :items="taglist" label="Select a tag" @update:modelValue="setTag" />
    <v-list>
        <v-list-item v-for="i in (imageCount - (page - 1) * 10) > 10 ? 10 : (imageCount - (page - 1) * 10)" :key="i">
            <v-list-item-title>Image #{{ imageIds[(page - 1) * 10 + i - 1] }}</v-list-item-title>
            <template v-slot:append>
                <v-btn icon="mdi-eye" @click="viewImage(imageIds[(page - 1) * 10 + i - 1])" />
            </template>
        </v-list-item>
    </v-list>
    <v-pagination v-model="page" :length="Math.ceil(imageCount / 10)" @update:modelValue="page = $event" />
</template>