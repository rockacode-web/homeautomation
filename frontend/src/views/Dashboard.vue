<template>
  <VContainer class="page-shell">
    <div class="panel">
      <VRow>
        <VCol cols="12" md="2">
          <VSheet class="widget slider-sheet d-flex flex-column align-center justify-center" color="#dbead8">
            <VSlider
              v-model="waterHeight"
              readonly
              color="#4caf50"
              track-color="#9ccc65"
              thumb-color="#4caf50"
              min="0"
              max="95"
              step="0.1"
              direction="vertical"
              thumb-label="always"
              class="level-slider"
            />
            <div class="text-caption mt-2">Height(in)</div>
          </VSheet>
        </VCol>

        <VCol cols="12" md="10">
          <VSheet class="widget pa-3">
            <div ref="areaChartRef" class="chart-box area-chart" />
          </VSheet>
        </VCol>
      </VRow>

      <VRow class="mt-1">
        <VCol cols="12" md="8">
          <VSheet class="widget pa-3">
            <div ref="gaugeChartRef" class="chart-box gauge-chart" />
          </VSheet>
        </VCol>

        <VCol cols="12" md="4">
          <VSheet class="widget pa-5 text-center">
            <h3 class="text-primary mb-1">Water Level</h3>
            <div class="text-caption text-medium-emphasis mb-3">Capacity Remaining</div>
            <div ref="fluidRef" class="fluid-meter-wrap mx-auto" />
          </VSheet>
        </VCol>
      </VRow>

      <VDialog v-model="overflowDialog" max-width="420">
        <VCard>
          <VCardTitle class="text-h6">Overflow Detected</VCardTitle>
          <VCardText>
            Water percentage exceeded 100% ({{ percentage.toFixed(1) }}%). Check inlet and overflow valve state.
          </VCardText>
          <VCardActions>
            <VSpacer />
            <VBtn color="primary" variant="tonal" @click="overflowDialog = false">Dismiss</VBtn>
          </VCardActions>
        </VCard>
      </VDialog>
    </div>
  </VContainer>
</template>

<script setup>
import { ref, watch, onMounted, onBeforeUnmount, nextTick } from 'vue'
import Highcharts from 'highcharts'
import HighchartsMore from 'highcharts/highcharts-more'
import { useMqttStore } from '@/store/mqttStore'

HighchartsMore(Highcharts)

// *** FIXED: use correct student ID so MQTT topic matches what backend publishes to ***
const STUDENT_ID = '620171712'
const LIVE_TOPIC = STUDENT_ID
const FALLBACK_TOPIC = `${STUDENT_ID}_sub`
const MAX_POINTS = 600

const mqttStore = useMqttStore()

const waterHeight = ref(0)
const reserve = ref(0)
const percentage = ref(0)
const overflowDialog = ref(false)

const areaChartRef = ref(null)
const gaugeChartRef = ref(null)
const fluidRef = ref(null)

let areaChart = null
let gaugeChart = null
let fluidMeter = null

const toNumber = (value, fallback = 0) => {
  const number = Number(value)
  return Number.isFinite(number) ? number : fallback
}

const updateAreaChart = (timestampMs, reserveValue) => {
  if (!areaChart) return
  areaChart.series[0].addPoint(
    [timestampMs, reserveValue],
    true,
    areaChart.series[0].data.length >= MAX_POINTS,
  )
}

const updateGaugeChart = (reserveValue) => {
  if (!gaugeChart) return
  gaugeChart.series[0].points[0].update(reserveValue)
}

const updateFluid = (percentValue) => {
  if (fluidMeter) {
    fluidMeter.setPercentage(Math.min(100, Math.max(0, percentValue)))
  }
}

const processPayload = (payload) => {
  if (!payload || typeof payload !== 'object') return

  const radar = toNumber(payload.radar, 0)
  waterHeight.value = toNumber(payload.waterheight, Math.max(0, 94.5 - radar))
  reserve.value = toNumber(payload.reserve, 0)
  percentage.value = toNumber(payload.percentage, 0)

  const timestamp = toNumber(payload.timestamp, Math.floor(Date.now() / 1000)) * 1000

  updateAreaChart(timestamp, reserve.value)
  updateGaugeChart(reserve.value)
  updateFluid(percentage.value)

  if (percentage.value > 100) {
    overflowDialog.value = true
  }
}

const initializeAreaChart = () => {
  areaChart = Highcharts.chart(areaChartRef.value, {
    chart: { zoomType: 'x' },
    title: { text: 'Water Reserves(10 min)', align: 'left' },
    tooltip: { shared: true },
    xAxis: {
      type: 'datetime',
      title: { text: 'Time' },
    },
    yAxis: {
      title: { text: 'Water level' },
      labels: { format: '{value} Gal' },
      min: 0,
      max: 1100,
    },
    legend: { enabled: true },
    credits: { enabled: true },
    series: [{
      name: 'Water',
      type: 'area',
      data: [],
      color: '#5db0e6',
    }],
  })
}

const initializeGaugeChart = () => {
  gaugeChart = Highcharts.chart(gaugeChartRef.value, {
    title: { text: 'Water Reserves', align: 'left' },
    yAxis: {
      min: 0,
      max: 1000,
      tickPixelInterval: 72,
      tickPosition: 'inside',
      tickColor: Highcharts.defaultOptions.chart.backgroundColor || '#FFFFFF',
      tickLength: 20,
      tickWidth: 2,
      minorTickInterval: null,
      labels: { distance: 20, style: { fontSize: '14px' } },
      lineWidth: 0,
      plotBands: [
        { from: 0, to: 200, color: '#DF5353', thickness: 20 },
        { from: 200, to: 600, color: '#DDDF0D', thickness: 20 },
        { from: 600, to: 1000, color: '#55BF3B', thickness: 20 },
      ],
    },
    tooltip: { shared: true },
    pane: {
      startAngle: -90,
      endAngle: 89.9,
      background: null,
      center: ['50%', '75%'],
      size: '110%',
    },
    series: [{
      type: 'gauge',
      name: 'Water Capacity',
      data: [0],
      tooltip: { valueSuffix: ' Gal' },
      dataLabels: {
        format: '{y} Gal',
        borderWidth: 0,
        color: (
          Highcharts.defaultOptions.title &&
          Highcharts.defaultOptions.title.style &&
          Highcharts.defaultOptions.title.style.color
        ) || '#333333',
        style: { fontSize: '16px' },
      },
      dial: { radius: '80%', backgroundColor: 'gray', baseWidth: 12, baseLength: '0%', rearLength: '0%' },
      pivot: { backgroundColor: 'gray', radius: 6 },
    }],
    credits: { enabled: true },
  })
}

const initializeFluidMeter = () => {
  if (!fluidRef.value || typeof window.FluidMeter !== 'function') return

  fluidRef.value.innerHTML = ''
  fluidMeter = new window.FluidMeter()
  fluidMeter.init({
    targetContainer: fluidRef.value,
    fillPercentage: 0,
    options: {
      size: 210,
      borderWidth: 18,
      backgroundColor: '#e6e8ef',
      foregroundColor: '#f6f7fb',
      fontSize: '58px',
      foregroundFluidLayer: { fillStyle: '#8a18ad' },
      backgroundFluidLayer: { fillStyle: '#6f0094' },
    },
  })
}

onMounted(async () => {
  await nextTick()
  initializeAreaChart()
  initializeGaugeChart()
  initializeFluidMeter()

  mqttStore.connect()
  // Wait for connection to establish before subscribing
  setTimeout(() => {
    mqttStore.subscribe(LIVE_TOPIC)
    mqttStore.subscribe(FALLBACK_TOPIC)
  }, 3000)
})

onBeforeUnmount(() => {
  try {
    mqttStore.unsubscribe(LIVE_TOPIC)
    mqttStore.unsubscribe(FALLBACK_TOPIC)
  } catch (error) {
    console.error(error)
  }
})

watch(
  () => mqttStore.payload,
  (payload) => processPayload(payload),
  { deep: true, immediate: true },
)
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

.widget {
  border: 1px solid #1a1a1a;
  background: #f3f3f3;
  min-height: 250px;
}

.slider-sheet {
  min-height: 330px;
}

.level-slider {
  height: 250px;
}

.chart-box {
  width: 100%;
}

.area-chart {
  min-height: 300px;
}

.gauge-chart {
  min-height: 250px;
}

.fluid-meter-wrap {
  min-height: 220px;
  width: 220px;
}
</style>