package com.zelgius.lightcontroller.ui.lights

import androidx.compose.animation.Animatable
import androidx.compose.animation.AnimatedVisibility
import androidx.compose.animation.core.Animatable
import androidx.compose.animation.core.LinearEasing
import androidx.compose.animation.core.animateFloatAsState
import androidx.compose.animation.core.tween
import androidx.compose.animation.fadeIn
import androidx.compose.animation.fadeOut
import androidx.compose.animation.scaleIn
import androidx.compose.animation.scaleOut
import androidx.compose.foundation.LocalIndication
import androidx.compose.foundation.combinedClickable
import androidx.compose.foundation.gestures.awaitEachGesture
import androidx.compose.foundation.gestures.awaitFirstDown
import androidx.compose.foundation.gestures.detectTapGestures
import androidx.compose.foundation.gestures.waitForUpOrCancellation
import androidx.compose.foundation.indication
import androidx.compose.foundation.interaction.Interaction
import androidx.compose.foundation.interaction.MutableInteractionSource
import androidx.compose.foundation.interaction.PressInteraction
import androidx.compose.foundation.interaction.collectIsPressedAsState
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.LazyItemScope
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.lazy.rememberLazyListState
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Delete
import androidx.compose.material.icons.twotone.Add
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.Button
import androidx.compose.material3.Card
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.ExperimentalMaterial3ExpressiveApi
import androidx.compose.material3.FloatingActionButton
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.material3.adaptive.ExperimentalMaterial3AdaptiveApi
import androidx.compose.material3.adaptive.currentWindowAdaptiveInfoV2
import androidx.compose.material3.adaptive.layout.calculatePaneScaffoldDirective
import androidx.compose.material3.adaptive.navigation3.ListDetailSceneStrategy
import androidx.compose.material3.adaptive.navigation3.rememberListDetailSceneStrategy
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.derivedStateOf
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.graphicsLayer
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.unit.dp
import androidx.navigation3.runtime.NavKey
import androidx.navigation3.runtime.entryProvider
import androidx.navigation3.runtime.rememberNavBackStack
import androidx.navigation3.ui.NavDisplay
import androidx.savedstate.serialization.SavedStateConfiguration
import com.zelgius.lightcontroller.domain.repository.web.Group
import com.zelgius.lightcontroller.domain.repository.web.Light
import com.zelgius.lightcontroller.ui.common.AppPullToRefresh
import com.zelgius.lightcontroller.ui.lights.add.AddItemScreen
import com.zelgius.lightcontroller.ui.lights.details.GroupDetailsScreen
import com.zelgius.lightcontroller.ui.lights.details.GroupDetailsViewModel
import com.zelgius.lightcontroller.utils.getContrastColor
import com.zelgius.lightcontroller.utils.xyToColor
import kotlinx.coroutines.async
import kotlinx.coroutines.awaitAll
import kotlinx.coroutines.launch
import kotlinx.serialization.ExperimentalSerializationApi
import kotlinx.serialization.SerialName
import kotlinx.serialization.Serializable
import kotlinx.serialization.modules.SerializersModule
import kotlinx.serialization.modules.polymorphic
import lightcontroller.composeapp.generated.resources.Res
import lightcontroller.composeapp.generated.resources.add
import lightcontroller.composeapp.generated.resources.cancel
import lightcontroller.composeapp.generated.resources.confirm_delete_group
import lightcontroller.composeapp.generated.resources.confirm_delete_group_title
import lightcontroller.composeapp.generated.resources.yes
import org.jetbrains.compose.resources.stringResource
import org.koin.compose.viewmodel.koinViewModel
import kotlin.time.Clock

@Composable
fun LightScreen(viewModel: LightViewModel = koinViewModel()) {
    LaunchedEffect(Unit) {
        viewModel.initialize()
    }

    val state by viewModel.state.collectAsState()

    LightScreen(state, onRefresh = viewModel::refresh, viewModel::deleteGroup)
}

@OptIn(ExperimentalMaterial3AdaptiveApi::class, ExperimentalSerializationApi::class)
@Composable
private fun LightScreen(state: LightState, onRefresh: () -> Unit, onDeleteGroup: (Group) -> Unit) {
    val config = SavedStateConfiguration {
        serializersModule = SerializersModule {
            polymorphic(NavKey::class) { // Use NavKey here as the base
                subclass(Route.Groups::class, Route.Groups.serializer())
                subclass(Route.Lights::class, Route.Lights.serializer())
                subclass(Route.AddItem::class, Route.AddItem.serializer())
            }
        }
    }


    val windowAdaptiveInfo = currentWindowAdaptiveInfoV2()
    val directive = remember(windowAdaptiveInfo) {
        calculatePaneScaffoldDirective(windowAdaptiveInfo)
    }

    val listDetailSceneStrategy = rememberListDetailSceneStrategy<NavKey>(
        directive = directive
    )

    val isThreePanes = directive.maxHorizontalPartitions >= 2

    val backStack = rememberNavBackStack(config, Route.Groups)

    val groupDetailsViewModel: GroupDetailsViewModel = koinViewModel()
    NavDisplay(
        backStack = backStack,
        sceneStrategies = listOf(listDetailSceneStrategy),
        onBack = { backStack.removeLastOrNull() },
        entryProvider = entryProvider {
            entry<Route.Groups>(metadata = ListDetailSceneStrategy.listPane()) {
                GroupList(state, onRefresh, onGroupClicked = { g ->
                    backStack.add(Route.Lights(group = g))
                    groupDetailsViewModel.initialize(g)
                }, onGroupDeleted = onDeleteGroup)
            }

            entry<Route.Lights>(metadata = ListDetailSceneStrategy.detailPane()) { route ->
                GroupDetailsScreen(viewModel = groupDetailsViewModel, group = route.group) {
                    backStack.add(Route.AddItem(route.group, it))
                }
            }

            entry<Route.AddItem>(metadata = ListDetailSceneStrategy.extraPane()) {
                AddItemScreen(group = it.group, lights = it.lights, onBack = {
                    backStack.removeLastOrNull()
                    Unit
                }.takeIf { isThreePanes }) { items ->
                    backStack.removeLastOrNull()
                    groupDetailsViewModel.onItemsAdded(items)
                }
            }

        }
    )
}

@OptIn(ExperimentalMaterial3ExpressiveApi::class)
@Composable
private fun GroupList(
    state: LightState,
    onRefresh: () -> Unit,
    onGroupClicked: (Group) -> Unit,
    onGroupDeleted: (Group) -> Unit
) {
    val listState = rememberLazyListState()
    var deleteGroup: Group? by remember { mutableStateOf(null) }
    Scaffold(
        floatingActionButton = {
            val isScrolling by remember {
                derivedStateOf { listState.isScrollInProgress }
            }

            AnimatedVisibility(
                !isScrolling,
                enter = fadeIn() + scaleIn(),
                exit = fadeOut() + scaleOut()
            ) {
                FloatingActionButton(onClick = {
                    onGroupClicked(Group(state.groups.maxOf { it.id } + 1, "New Group", 0f, 0f, 0f))
                }) {
                    Icon(Icons.TwoTone.Add, contentDescription = stringResource(Res.string.add))
                }
            }
        }
    ) {
        AppPullToRefresh(
            modifier = Modifier.fillMaxSize().padding(it),
            isRefreshing = state.isLoading,
            onRefresh = onRefresh,

            ) {
            LazyColumn(
                modifier = Modifier.fillMaxSize().padding(8.dp),
                state = listState,
                verticalArrangement = Arrangement.spacedBy(4.dp)
            ) {
                items(state.groups, key = { group -> group.id }) {

                    val container =
                        if (it.x == 0f && it.y == 0f) CardDefaults.cardColors().containerColor else xyToColor(
                            it.x, it.y, it.brightness
                        )

                    GroupCard(
                        group = it,
                        container = container,
                        onGroupClicked = {
                            onGroupClicked(it)
                        },
                        onDeleteClicked = {
                            deleteGroup = it
                        })
                }

                item {
                    Box(modifier = Modifier.height(64.dp))
                }
            }
        }

        deleteGroup?.let { g ->
            AlertDialog(
                onDismissRequest = { deleteGroup = null },
                confirmButton = {
                    Button(onClick = {
                        onGroupDeleted(g)
                        deleteGroup = null
                    }) {
                        Text(stringResource(Res.string.yes))
                    }
                },
                dismissButton = {
                    TextButton(onClick = { deleteGroup = null }) {
                        Text(stringResource(Res.string.cancel))
                    }
                },
                title = {
                    Text(stringResource(Res.string.confirm_delete_group_title, g.name))
                },
                text = {
                    Text(stringResource(Res.string.confirm_delete_group))
                },
            )
        }
    }
}

@Composable
private fun LazyItemScope.GroupCard(
    group: Group,
    container: Color,
    onGroupClicked: (Group) -> Unit,
    onDeleteClicked: (Group) -> Unit
) {
    val progress = remember { Animatable(0f) }
    val progressColor = remember { Animatable(container) }
    val scope = rememberCoroutineScope()

    // 1. Setup InteractionSource and Indication
    val deleteColor = MaterialTheme.colorScheme.errorContainer
    val interactionSource = remember { MutableInteractionSource() }
    val indication = LocalIndication.current // Gets the platform-default ripple

    Card(
        modifier = Modifier
            .animateItem()
            .fillMaxWidth()
            .indication(interactionSource, indication)
            .pointerInput(group) {
                awaitEachGesture {
                    val down = awaitFirstDown()
                    val startTime = Clock.System.now().toEpochMilliseconds()

                    // 3. Manually trigger the Ripple start
                    val pressInteraction = PressInteraction.Press(down.position)
                    scope.launch { interactionSource.emit(pressInteraction) }

                    val animationJob = scope.launch {
                        val alphaProgress = async {
                            progress.animateTo(1f, tween(1000, easing = LinearEasing))
                        }
                        val colorProgress = async {
                            progressColor.animateTo(deleteColor, tween(1000, easing = LinearEasing))
                        }

                        awaitAll(alphaProgress, colorProgress)

                        onDeleteClicked(group)
                        interactionSource.emit(PressInteraction.Release(pressInteraction))
                        progress.animateTo(0f, tween(100, easing = LinearEasing))
                        progressColor.animateTo(container, tween(100, easing = LinearEasing))
                    }

                    val up = waitForUpOrCancellation()
                    animationJob.cancel()

                    // 4. Manually trigger the Ripple end (Release or Cancel)
                    scope.launch {
                        if (up != null) {
                            interactionSource.emit(PressInteraction.Release(pressInteraction))

                            val elapsed = Clock.System.now().toEpochMilliseconds() - startTime
                            if (elapsed < 300) onGroupClicked(group)
                        } else {
                            interactionSource.emit(PressInteraction.Cancel(pressInteraction))
                        }
                    }

                    // Reset icon if long press wasn't completed
                    scope.launch {
                        if (progress.value < 1f) {
                            progress.animateTo(0f)
                            progressColor.animateTo(container)
                        }
                    }
                }
            },
        colors = CardDefaults.cardColors(containerColor = progressColor.value),
    ) {
        Row(
            modifier = Modifier.padding(16.dp).fillMaxWidth(),
            verticalAlignment = Alignment.CenterVertically
        ) {
            Text(
                text = group.name,
                modifier = Modifier.weight(1f),
                color = container.getContrastColor()
            )

            Icon(
                imageVector = Icons.Default.Delete,
                contentDescription = null,
                tint = container.getContrastColor(),
                modifier = Modifier.graphicsLayer { alpha = progress.value }
            )
        }
    }
}

@Serializable
private sealed interface Route : NavKey {

    @Serializable
    @SerialName("Groups")
    data object Groups : Route

    @Serializable
    @SerialName("Lights")
    data class Lights(val group: Group) : Route

    @Serializable
    @SerialName("AddItem")
    data class AddItem(val group: Group, val lights: List<Light>) : Route
}