package com.zelgius.lightcontroller

import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.BoxWithConstraints
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.twotone.Dashboard
import androidx.compose.material.icons.twotone.Image
import androidx.compose.material.icons.twotone.Lightbulb
import androidx.compose.material.icons.twotone.Settings
import androidx.compose.material3.Icon
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.NavigationBarDefaults
import androidx.compose.material3.NavigationBarItemDefaults
import androidx.compose.material3.NavigationDrawerItemColors
import androidx.compose.material3.NavigationDrawerItemDefaults
import androidx.compose.material3.NavigationItemColors
import androidx.compose.material3.NavigationRailItemDefaults
import androidx.compose.material3.Text
import androidx.compose.material3.adaptive.ExperimentalMaterial3AdaptiveApi
import androidx.compose.material3.adaptive.currentWindowAdaptiveInfo
import androidx.compose.material3.adaptive.currentWindowAdaptiveInfoV2
import androidx.compose.material3.adaptive.layout.calculatePaneScaffoldDirective
import androidx.compose.material3.adaptive.navigation.BackNavigationBehavior
import androidx.compose.material3.adaptive.navigation3.ListDetailSceneStrategy
import androidx.compose.material3.adaptive.navigation3.SupportingPaneSceneStrategy
import androidx.compose.material3.adaptive.navigation3.rememberListDetailSceneStrategy
import androidx.compose.material3.adaptive.navigation3.rememberSupportingPaneSceneStrategy
import androidx.compose.material3.adaptive.navigationsuite.NavigationSuiteDefaults
import androidx.compose.material3.adaptive.navigationsuite.NavigationSuiteScaffold
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.vector.ImageVector
import androidx.compose.ui.unit.dp
import androidx.navigation3.runtime.NavKey
import androidx.navigation3.runtime.entryProvider
import androidx.navigation3.ui.NavDisplay
import com.zelgius.lightcontroller.navigation.Home
import com.zelgius.lightcontroller.navigation.Image
import com.zelgius.lightcontroller.navigation.Lights
import com.zelgius.lightcontroller.navigation.Placeholder
import com.zelgius.lightcontroller.navigation.Settings
import com.zelgius.lightcontroller.ui.home.HomeScreen
import com.zelgius.lightcontroller.ui.image.ImageScreen
import com.zelgius.lightcontroller.ui.lights.LightScreen
import com.zelgius.lightcontroller.ui.settings.SettingsScreen
import com.zelgius.lightcontroller.ui.theme.AppTheme
import lightcontroller.composeapp.generated.resources.Res
import lightcontroller.composeapp.generated.resources.dashboard
import lightcontroller.composeapp.generated.resources.image
import lightcontroller.composeapp.generated.resources.lights
import lightcontroller.composeapp.generated.resources.settings
import org.jetbrains.compose.resources.StringResource
import org.jetbrains.compose.resources.stringResource

enum class AppDestinations(
    val label: StringResource,
    val icon: ImageVector,
    val contentDescription: StringResource
) {
    Dashboard(Res.string.dashboard, Icons.TwoTone.Dashboard, Res.string.dashboard),
    Lights(Res.string.lights, Icons.TwoTone.Lightbulb, Res.string.lights),
    Image(Res.string.image, Icons.TwoTone.Image, Res.string.image),
    Settings(Res.string.settings, Icons.TwoTone.Settings, Res.string.settings),
}

@OptIn(ExperimentalMaterial3AdaptiveApi::class)
@Composable
fun App() {
    val backStack = createBackStack()
    var currentDestination: AppDestinations? by rememberSaveable { mutableStateOf(null) }

    AppTheme {
        BoxWithConstraints {
            val windowAdaptiveInfo = currentWindowAdaptiveInfoV2()
            val directive = remember(windowAdaptiveInfo) {
                calculatePaneScaffoldDirective(windowAdaptiveInfo)
                    .copy(
                        horizontalPartitionSpacerSize = 0.dp,
                        maxVerticalPartitions = 1,
                    )
            }

            val isSinglePane = directive.maxHorizontalPartitions == 1


            val itemsColors = NavigationSuiteDefaults.itemColors(
                navigationRailItemColors = NavigationRailItemDefaults.colors(
                    selectedIconColor = MaterialTheme.colorScheme.secondary
                ),
                navigationDrawerItemColors = NavigationDrawerItemDefaults.colors(
                    selectedIconColor = MaterialTheme.colorScheme.secondary
                ),
                navigationBarItemColors = NavigationBarItemDefaults.colors(
                    selectedIconColor = MaterialTheme.colorScheme.secondary
                )
            )
            NavigationSuiteScaffold(
                navigationSuiteItems = {
                    val entries = buildList {
                        if (isSinglePane) add(AppDestinations.Dashboard)
                        add(AppDestinations.Lights)
                        add(AppDestinations.Image)
                        add(AppDestinations.Settings)
                    }
                    entries.forEach {
                        item(
                            icon = {
                                Icon(
                                    it.icon,
                                    contentDescription = stringResource(it.contentDescription),
                                )
                            },
                            colors = itemsColors,
                            label = { Text(stringResource(it.label)) },
                            selected = it == currentDestination,
                            onClick = {
                                currentDestination = it

                                backStack.add(
                                    when (it) {
                                        AppDestinations.Dashboard -> Home
                                        AppDestinations.Lights -> Lights
                                        AppDestinations.Image -> Image
                                        AppDestinations.Settings -> Settings
                                    }
                                )
                            }
                        )
                    }
                }
            ) {

                val supportingPaneStrategy = rememberListDetailSceneStrategy<NavKey>(
                    backNavigationBehavior = BackNavigationBehavior.PopUntilCurrentDestinationChange,
                    directive = directive
                )

                NavDisplay(
                    backStack = backStack,
                    sceneStrategies = listOf(supportingPaneStrategy),
                    onBack = { backStack.removeLastOrNull() },
                    entryProvider = entryProvider {
                        entry<Home>(
                            metadata = ListDetailSceneStrategy.listPane()
                        ) {
                            currentDestination = AppDestinations.Dashboard
                            HomeScreen(isSinglePane) {
                                backStack.add(it)
                            }
                        }

                        entry<Settings>(
                            metadata = ListDetailSceneStrategy.detailPane()
                        ) {
                            currentDestination = AppDestinations.Settings
                            SettingsScreen(
                                onBack = ({ backStack.removeLastOrNull(); Unit }).takeIf { isSinglePane }
                            )
                        }

                        entry<Lights>(
                            metadata = ListDetailSceneStrategy.detailPane()
                        ) {
                            currentDestination = AppDestinations.Lights
                            LightScreen()
                        }


                        entry<Image>(
                            metadata = ListDetailSceneStrategy.detailPane()
                        ) {
                            currentDestination = AppDestinations.Image
                            ImageScreen(
                                onBack = ({ backStack.removeLastOrNull(); Unit }).takeIf { isSinglePane }
                            )
                        }

                    }
                )
            }
        }
    }

}