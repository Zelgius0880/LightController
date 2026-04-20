package com.zelgius.lightcontroller.navigation

import androidx.navigation3.runtime.NavKey
import androidx.savedstate.serialization.SavedStateConfiguration
import kotlinx.serialization.Serializable
import kotlinx.serialization.modules.SerializersModule
import kotlinx.serialization.modules.polymorphic

@Serializable
sealed class Route(val name: String): NavKey {
    companion object {
        fun fromName(name: String): Route? =
            when (name) {
                Home.name -> Home
                Settings.name -> Settings
                Placeholder.name -> Placeholder
                else -> Home
            }
    }
}

@Serializable
data object Home : Route("home")

@Serializable
data object Settings : Route("settings")



@Serializable
data object Placeholder : Route("placeholder")

