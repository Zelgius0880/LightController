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
                Lights.name -> Lights
                Image.name -> Image
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



@Serializable
data object Lights : Route("lights")


@Serializable
data object Image : Route("image")

