<!-- we will show a chip if there are messages from ws, read shouldReload prop from  main ref -->

<script setup>
import { ref } from 'vue'

const msgCount = ref(0)

const bus = new BroadcastChannel('reload')

const ws = new WebSocket('ws://localhost:8765/notify')
ws.onmessage = (event) => {
    msgCount.value = msgCount.value + 1
}

</script>

<template>
    <v-chip v-if="msgCount > 0" color="red" text-color="white" @click="msgCount = 0; bus.postMessage('reload')" class="ma-2">
        {{ msgCount }} new images
    </v-chip>
</template>