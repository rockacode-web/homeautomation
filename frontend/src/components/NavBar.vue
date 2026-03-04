<template>
  <VToolbar flat density="comfortable" color="surface" class="nav-toolbar">
    <VContainer fluid>
      <VRow align="center" justify="center">
        <VCol cols="1" />

        <VCol cols="10" class="text-center">
          <RouterLink v-for="item in links" :key="item.name" class="route" :to="{ name: item.name }">
            <VBtn
              class="text-caption font-weight-medium"
              density="compact"
              size="small"
              color="primary"
              :variant="route.name === item.name ? 'tonal' : 'text'"
            >
              {{ item.label }}
            </VBtn>
          </RouterLink>
        </VCol>

        <VCol cols="1" class="text-right">
          <VIcon icon="mdi-star-four-points" size="16" />
        </VCol>
      </VRow>
    </VContainer>
  </VToolbar>
</template>

<script setup>
import { useRoute } from 'vue-router'

const route = useRoute()

const links = [
  { name: 'Home', label: 'Home' },
  { name: 'Control', label: 'Control' },
  { name: 'Dashboard', label: 'Dashboard' },
  { name: 'Analysis', label: 'Analysis' },
]
</script>

<style scoped>
.nav-toolbar {
  border-bottom: 1px solid #d5d9df;
}

.route {
  text-decoration: none;
  margin: 0 8px;
}
</style>
  
