<template>
  <VContainer class="page-shell">
    <div class="panel">
      <VRow justify="center" class="mt-10">
        <VCol cols="12" md="7" lg="6">
          <VCard elevation="0" class="control-card">
            <VCardTitle class="text-h6 font-weight-bold">Remote Authentication Control</VCardTitle>
            <VCardSubtitle>Enter and save a 4-digit passcode for the gate lock system.</VCardSubtitle>

            <VCardText>
              <VOtpInput
                v-model="passcode"
                class="my-4"
                length="4"
                type="number"
                variant="outlined"
                :disabled="loading"
              />

              <VBtn
                block
                color="primary"
                variant="flat"
                :loading="loading"
                :disabled="passcode.length !== 4"
                @click="setCombination"
              >
                Set Combination
              </VBtn>

              <VAlert v-if="message" class="mt-4" :type="messageType" variant="tonal">{{ message }}</VAlert>
            </VCardText>
          </VCard>
        </VCol>
      </VRow>
    </div>
  </VContainer>
</template>

<script setup>
import { ref } from 'vue'

const passcode = ref('')
const loading = ref(false)
const message = ref('')
const messageType = ref('info')

const setCombination = async () => {
  if (!/^\d{4}$/.test(passcode.value)) {
    message.value = 'Passcode must be exactly 4 digits.'
    messageType.value = 'warning'
    return
  }

  loading.value = true
  message.value = ''

  try {
    const form = new URLSearchParams()
    form.append('passcode', passcode.value)

    const response = await fetch('/api/set/combination', {
      method: 'POST',
      headers: {
        'Content-Type': 'application/x-www-form-urlencoded',
      },
      body: form.toString(),
    })

    const data = await response.json()
    if (response.ok && data.status === 'complete') {
      message.value = 'Passcode saved successfully.'
      messageType.value = 'success'
    } else {
      message.value = 'Failed to save passcode.'
      messageType.value = 'error'
    }
  } catch (error) {
    console.error(error)
    message.value = 'Unable to connect to backend API.'
    messageType.value = 'error'
  } finally {
    loading.value = false
  }
}
</script>

<style scoped>
.page-shell {
  max-width: 1120px;
}

.panel {
  background: #f2f3f5;
  border: 1px solid #7aa2ea;
  min-height: calc(100vh - 180px);
  padding: 20px;
}

.control-card {
  border: 1px solid #cfd4dc;
  background: #fff;
}
</style>
