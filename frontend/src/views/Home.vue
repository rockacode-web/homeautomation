<template>
  <VContainer class="page-shell">
    <div class="panel">
      <VRow align="center" justify="center" class="text-center py-12">
        <VCol cols="12" md="8">
          <VImg height="120" class="mx-auto mb-4" src="@/assets/logo.svg" />
          <h1 class="text-h4 font-weight-bold mb-3">Home Automation UI</h1>
          <p class="text-body-1 text-medium-emphasis mb-8">
            Use the navigation bar to configure passcode control, monitor live water reserves, and analyze stored data.
          </p>
          <VBtn color="primary" variant="tonal" size="large" :to="{ name: 'Dashboard' }">Open Dashboard</VBtn>
        </VCol>
      </VRow>
    </div>
  </VContainer>
</template>

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
</style>
