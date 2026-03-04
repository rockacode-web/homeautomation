<template>
  <VContainer class="page-shell">
    <div class="panel">
      <VRow class="mb-4">
        <VCol cols="12" md="3">
          <VCard class="filter-card" elevation="0">
            <VCardText>
              <VTextField v-model="startDate" type="date" label="Start Date" density="compact" variant="outlined" />
              <VTextField v-model="endDate" type="date" label="End Date" density="compact" variant="outlined" />
              <VBtn color="primary" block variant="tonal" :loading="loading" @click="analyze">Analyze</VBtn>
            </VCardText>
          </VCard>
        </VCol>

        <VCol cols="12" md="4">
          <VCard class="avg-card" elevation="0">
            <VCardText class="text-center">
              <div class="text-subtitle-1 font-weight-bold">Average</div>
              <div class="text-caption text-medium-emphasis">For the selected period</div>
              <div class="avg-value">{{ average.toFixed(1) }}<span class="unit"> Gal</span></div>
            </VCardText>
          </VCard>
        </VCol>
      </VRow>

      <VSheet class="widget pa-3 mb-4">
        <div ref="lineChartRef" class="chart-box line-chart" />
      </VSheet>

      <VSheet class="widget pa-3">
        <div ref="scatterChartRef" class="chart-box scatter-chart" />
      </VSheet>
    </div>
  </VContainer>
</template>

<script setup name="AnalysisView">
import { ref, nextTick, onMounted } from 'vue'
import Highcharts from 'highcharts'

const lineChartRef = ref(null)
const scatterChartRef = ref(null)

let lineChart = null
let scatterChart = null

// Default to today for both start and end since data was just inserted today
const today = new Date()
const startDate = ref(today.toISOString().slice(0, 10))
const endDate = ref(today.toISOString().slice(0, 10))
const loading = ref(false)
const average = ref(0)

// Use Z suffix to force UTC interpretation — avoids timezone offset issues
const toUnixRange = () => {
  const start = Math.floor(new Date(`${startDate.value}T00:00:00Z`).getTime() / 1000)
  const end = Math.floor(new Date(`${endDate.value}T23:59:59Z`).getTime() / 1000)
  return { start, end }
}

const normalizeAverage = (data) => {
  if (typeof data === 'number') {
    return data
  }
  if (Array.isArray(data) && data.length > 0) {
    const value = data[0].average ?? data[0].avg ?? 0
    return Number(value) || 0
  }
  if (data && typeof data === 'object') {
    return Number(data.average ?? data.avg ?? 0) || 0
  }
  return 0
}

const normalizeReserveSeries = (data) => {
  if (!Array.isArray(data)) {
    return []
  }
  return data
    .map((item) => [Number(item.timestamp) * 1000, Number(item.reserve)])
    .filter((point) => Number.isFinite(point[0]) && Number.isFinite(point[1]))
}

const normalizeScatterSeries = (data) => {
  if (!Array.isArray(data)) {
    return []
  }
  return data
    .map((item) => [Number(item.waterheight), Number(item.radar)])
    .filter((point) => Number.isFinite(point[0]) && Number.isFinite(point[1]))
}

const initializeLineChart = () => {
  lineChart = Highcharts.chart(lineChartRef.value, {
    title: { text: 'Water Management Analysis', align: 'left' },
    chart: { zoomType: 'x' },
    xAxis: { type: 'datetime', title: { text: 'Time' } },
    yAxis: {
      title: { text: 'Water Reserves' },
      labels: { format: '{value} Gal' },
    },
    legend: { enabled: true },
    tooltip: { shared: true },
    series: [{ name: 'Reserve', type: 'line', data: [], color: '#43aaf0' }],
    credits: { enabled: true },
  })
}

const initializeScatterChart = () => {
  scatterChart = Highcharts.chart(scatterChartRef.value, {
    title: { text: 'Height and Water Level Correlation Analysis', align: 'left' },
    chart: { type: 'scatter', zoomType: 'xy' },
    xAxis: { title: { text: 'Water Height' }, labels: { format: '{value} in' } },
    yAxis: { title: { text: 'Radar' }, labels: { format: '{value} in' } },
    legend: { enabled: true },
    tooltip: {
      pointFormat: 'Water Height: <b>{point.x:.2f} in</b><br/>Radar: <b>{point.y:.2f} in</b>',
    },
    series: [{ name: 'Analysis', type: 'scatter', data: [], color: '#43aaf0' }],
    credits: { enabled: true },
  })
}

const analyze = async () => {
  const { start, end } = toUnixRange()
  if (!Number.isFinite(start) || !Number.isFinite(end) || end < start) {
    return
  }

  loading.value = true
  try {
    const [reserveResponse, avgResponse] = await Promise.all([
      fetch(`/api/reserve/${start}/${end}`),
      fetch(`/api/avg/${start}/${end}`),
    ])

    const reserveJson = await reserveResponse.json()
    const avgJson = await avgResponse.json()

    const reserveData = reserveJson.status === 'found' ? reserveJson.data : []
    const avgData = avgJson.status === 'found' ? avgJson.data : 0

    average.value = normalizeAverage(avgData)
    lineChart.series[0].setData(normalizeReserveSeries(reserveData), true)
    scatterChart.series[0].setData(normalizeScatterSeries(reserveData), true)
  } catch (error) {
    console.error(error)
    average.value = 0
    lineChart.series[0].setData([], true)
    scatterChart.series[0].setData([], true)
  } finally {
    loading.value = false
  }
}

onMounted(async () => {
  await nextTick()
  initializeLineChart()
  initializeScatterChart()
  analyze()
})
</script>

<style scoped>
.page-shell {
  max-width: 1120px;
}

.panel {
  background: #f2f3f5;
  border: 1px solid #7aa2ea;
  min-height: calc(100vh - 180px);
  padding: 18px;
}

.filter-card,
.avg-card {
  border: 1px solid #d3d8df;
  background: #fff;
}

.avg-value {
  margin-top: 6px;
  font-size: 64px;
  line-height: 1;
}

.unit {
  font-size: 20px;
}

.widget {
  border: 1px solid #1a1a1a;
  background: #f3f3f3;
}

.chart-box {
  width: 100%;
}

.line-chart,
.scatter-chart {
  min-height: 280px;
}
</style>