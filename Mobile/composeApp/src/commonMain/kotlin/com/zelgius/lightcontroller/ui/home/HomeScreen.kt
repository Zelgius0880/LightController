package com.zelgius.lightcontroller.ui.home

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.FlowRow
import androidx.compose.foundation.layout.IntrinsicSize
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.RowScope
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.aspectRatio
import androidx.compose.foundation.layout.fillMaxHeight
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.itemsIndexed
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.AssistChip
import androidx.compose.material3.Card
import androidx.compose.material3.CircularWavyProgressIndicator
import androidx.compose.material3.ExperimentalMaterial3ExpressiveApi
import androidx.compose.material3.LoadingIndicator
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.remember
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.StrokeCap
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import com.tweener.chartopia.type.donut.DonutChart
import com.tweener.chartopia.type.donut.DonutChartDefault
import com.tweener.chartopia.type.donut.model.Segment
import com.tweener.kmpkit.kotlinextensions.degrees
import com.tweener.kmpkit.kotlinextensions.now
import com.zelgius.lightcontroller.domain.repository.web.WebSocketMessage
import com.zelgius.lightcontroller.navigation.Lights
import com.zelgius.lightcontroller.navigation.Route
import com.zelgius.lightcontroller.navigation.Settings
import com.zelgius.lightcontroller.ui.theme.AppTheme
import com.zelgius.lightcontroller.utils.dateFormat
import com.zelgius.lightcontroller.utils.timeFormat
import kotlinx.datetime.LocalDateTime
import kotlinx.datetime.TimeZone
import kotlinx.datetime.format
import kotlinx.datetime.toLocalDateTime
import lightcontroller.composeapp.generated.resources.Res
import lightcontroller.composeapp.generated.resources.disconnected
import lightcontroller.composeapp.generated.resources.file_system
import lightcontroller.composeapp.generated.resources.heap
import lightcontroller.composeapp.generated.resources.hue
import lightcontroller.composeapp.generated.resources.netatmo
import lightcontroller.composeapp.generated.resources.next_token
import lightcontroller.composeapp.generated.resources.psram
import lightcontroller.composeapp.generated.resources.waiting_data
import org.jetbrains.compose.resources.stringResource
import org.koin.compose.viewmodel.koinViewModel
import kotlin.time.Instant

@Composable
fun HomeScreen(
    isSinglePane: Boolean,
    viewModel: HomeViewModel = koinViewModel(), onRouteChanged: (Route) -> Unit
) {
    LaunchedEffect(Unit) {
        viewModel.init()
    }

    val state by viewModel.state.collectAsState()

    LaunchedEffect((state as? HomeState.Loaded)?.settingsSet) {
        if ((state as? HomeState.Loaded)?.settingsSet == false) onRouteChanged(Settings)
        else if((state as? HomeState.Loaded)?.settingsSet == true && !isSinglePane) {
            onRouteChanged(Lights)
        }
    }

    Home(state = state)
}

@OptIn(ExperimentalMaterial3ExpressiveApi::class)
@Composable
private fun Home(
    state: HomeState
) {
    Scaffold { padding ->
        when (state) {
            is HomeState.Loading -> Box(Modifier.fillMaxSize()) {
                CircularWavyProgressIndicator(modifier = Modifier.align(Alignment.Center))
            }

            is HomeState.Loaded -> {
                @Composable
                fun content() {
                    Column(modifier = Modifier.padding(padding)) {
                        Row(horizontalArrangement = Arrangement.spacedBy(4.dp)) {
                            val status = state.status
                            StatusChartItem(
                                stringResource(Res.string.file_system),
                                total = status.fsTotal,
                                used = status.fsUsed
                            )


                            StatusChartItem(
                                stringResource(Res.string.psram),
                                total = status.totalBytes,
                                used = status.usedBytes
                            )


                            StatusChartItem(
                                stringResource(Res.string.heap),
                                total = status.heapTotal,
                                used = status.heapTotal - status.heapFree
                            )

                        }
                        AuthenticationStatus(state)
                        Logs(state.logs)

                    }
                }

                if (state.connected) content()
                else {
                    content()
                    Box(
                        Modifier.padding(padding).fillMaxSize()
                            .background(MaterialTheme.colorScheme.surface.copy(.6f))
                    ) {
                        Text(
                            stringResource(Res.string.disconnected), modifier = Modifier.align(
                                Alignment.Center
                            ), style = MaterialTheme.typography.headlineMedium.copy(
                                color = MaterialTheme.colorScheme.error
                            )
                        )
                    }
                }
            }
        }
    }
}


@Composable
private fun Logs(logs: List<WebSocketMessage.Log>) {
    Box(
        modifier = Modifier.clip(RoundedCornerShape(topStart = 16.dp, topEnd = 16.dp))
            .background(MaterialTheme.colorScheme.surfaceContainer).fillMaxSize().padding(8.dp),
    ) {
        LazyColumn(
            Modifier.fillMaxSize(), verticalArrangement = Arrangement.spacedBy(4.dp)
        ) {
            itemsIndexed(
                logs, key = { index, log -> "$index$log" }) { _, log ->
                Card(modifier = Modifier.fillMaxWidth().animateItem()) {
                    Column(modifier = Modifier.padding(8.dp)) {
                        val labelColor = MaterialTheme.typography.labelSmall.copy(
                            MaterialTheme.colorScheme.onSurfaceVariant.copy(alpha = .7f)
                        )
                        log.time?.let {
                            Row(
                                modifier = Modifier.fillMaxWidth(),
                                horizontalArrangement = Arrangement.SpaceBetween
                            ) {
                                Text(dateFormat.format(it), style = labelColor)
                                Text(timeFormat.format(it), style = labelColor)
                            }
                        } ?: run {
                            Text("<N/A>", style = labelColor)
                        }

                        Text(log.message)
                    }

                }

            }

        }
    }
}

@Composable
private fun AuthenticationStatus(
    state: HomeState.Loaded
) {
    FlowRow(modifier = Modifier.padding(horizontal = 16.dp, vertical = 8.dp).fillMaxWidth(), horizontalArrangement = Arrangement.SpaceAround) {
        Column(horizontalAlignment = Alignment.CenterHorizontally) {
            val netatmo = state.status.netatmo
            ConnectionChip(
                text = stringResource(Res.string.netatmo),
                connected = if (state.connected)
                    netatmo.authenticated && netatmo.valid
                else null
            )
            val time = Instant.fromEpochSeconds(netatmo.creationTimestamp + netatmo.expireIn)
                .toLocalDateTime(
                    TimeZone.currentSystemDefault()
                )
            Text(
                "${stringResource(Res.string.next_token)}: ${dateFormat.format(time)} ${
                    timeFormat.format(
                        time
                    )
                }",
                style = MaterialTheme.typography.labelMedium.copy(
                    color = if(netatmo.valid) MaterialTheme.colorScheme.onSurfaceVariant
                    else MaterialTheme.colorScheme.error
                )
            )
        }

        ConnectionChip(
            text = stringResource(Res.string.hue),
            connected =  if (state.connected)
                state.status.authenticated
            else null
        )
    }
}

@Composable
private fun ConnectionChip(
    text: String, connected: Boolean?
) {
    AssistChip(onClick = {}, leadingIcon = {
        Box(
            modifier = Modifier.clip(CircleShape).size(8.dp).background(
                when (connected) {
                    true -> MaterialTheme.colorScheme.primary
                    false -> MaterialTheme.colorScheme.error
                    null -> MaterialTheme.colorScheme.surfaceVariant
                }
            )
        )
    }, label = {
        Text(text)
    })

}

@OptIn(ExperimentalMaterial3ExpressiveApi::class)
@Composable
fun RowScope.StatusChartItem(title: String, total: Int, used: Int) {
    val safeTotal = if (total <= 0) 1 else total
    val usedRatio = used.toFloat() / safeTotal.toFloat()
    val freeRatio = (safeTotal - used).toFloat() / safeTotal.toFloat()

    val segments = listOf(
        Segment(
            angle = (usedRatio * 360f).degrees, baseColor = Color(0xFFEF5350), // Red for Used
            id = "Used"
        ), Segment(
            angle = (freeRatio * 360f).degrees, baseColor = Color(0xFF66BB6A), // Green for Free
            id = "Free"
        )
    )

    Box(modifier = Modifier.weight(1f).height(IntrinsicSize.Max)) {
        Column(
            horizontalAlignment = Alignment.CenterHorizontally, modifier = Modifier.fillMaxSize()
        ) {
            Text(title, style = MaterialTheme.typography.titleLarge)
            Spacer(modifier = Modifier.height(16.dp))

            DonutChart(
                modifier = Modifier.fillMaxWidth().aspectRatio(1f),
                segments = segments,
                sizes = DonutChartDefault.sizes(strokeWidth = 30.dp),
                strokeCap = StrokeCap.Butt,
                startAngleFromOrigin = 270f.degrees // Starts from the top
            )

            Spacer(modifier = Modifier.height(8.dp))
            Text(
                text = "${(usedRatio * 100).toInt()}% Used",
                style = MaterialTheme.typography.bodyMedium
            )
            Text(
                text = formatBytes(used) + " / " + formatBytes(total),
                style = MaterialTheme.typography.labelSmall,
                color = Color.Gray
            )
        }

        if (total == 0) {
            Box(
                modifier = Modifier.background(MaterialTheme.colorScheme.surface.copy(alpha = 0.6f))
                    .fillMaxWidth().fillMaxHeight()
            ) {
                Column(
                    modifier = Modifier.align(Alignment.Center).padding(top = 24.dp),
                    horizontalAlignment = Alignment.CenterHorizontally
                ) {
                    LoadingIndicator()
                    Text(stringResource(Res.string.waiting_data))
                }
            }
        }
    }
}

fun formatBytes(bytes: Int): String {
    return when {
        bytes >= 1024 * 1024 -> "${bytes / (1024 * 1024)} MB"
        bytes >= 1024 -> "${bytes / 1024} KB"
        else -> "$bytes B"
    }
}

@Composable
@Preview
private fun HomeScreenPreview() {
    AppTheme {
        val state = remember {
            HomeState.Loaded(logs = List(5) {
                WebSocketMessage.Log(time = LocalDateTime.now(), "Log $it")
            } + WebSocketMessage.Log(time = null, "Not timed log"),
                connected = true,
                settingsSet = true,
                status = WebSocketMessage.Status(
                    authenticated = true,
                    username = "abc",
                    firmware = "1.0.0",
                    netatmo = WebSocketMessage.Status.Netatmo(
                        authenticated = true,
                        valid = true,
                        creationTimestamp = 1776847640,
                        expireIn = 3600
                    )
                ))
        }

        Home(state)
    }
}