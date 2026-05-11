package com.zelgius.lightcontroller

import com.zelgius.lightcontroller.data.ServerService
import com.zelgius.lightcontroller.domain.repository.settings.SettingsRepository
import com.zelgius.lightcontroller.domain.repository.web.Group
import com.zelgius.lightcontroller.domain.repository.web.Light
import com.zelgius.lightcontroller.domain.repository.web.LightType
import com.zelgius.lightcontroller.domain.repository.web.ServerRepository
import com.zelgius.lightcontroller.domain.repository.web.StateMode
import com.zelgius.lightcontroller.domain.repository.web.Switch
import com.zelgius.lightcontroller.domain.useCase.SaveItemsUseCase
import io.ktor.client.*
import io.ktor.client.engine.mock.*
import io.ktor.client.plugins.contentnegotiation.*
import io.ktor.http.*
import io.ktor.serialization.kotlinx.json.*
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.test.runTest
import kotlinx.serialization.json.Json
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertTrue

class FullSystemIntegrationTest {

    // 1. Expanded In-memory "Database"
    private val mockGroups = mutableListOf<Group>()
    private val mockLights = mutableListOf<Light>()
    private val mockSwitches = mutableListOf<Switch>()

    private val json = Json { ignoreUnknownKeys = true }

    private val mockSettingRepo = object : SettingsRepository {
        private val _ipFlow = MutableStateFlow("127.0.0.1")
        override val ipFlow = _ipFlow

        private val _portFlow = MutableStateFlow(81)
        override val portFlow: Flow<Int?>
            get() = _portFlow

        private val _hueApiKey = MutableStateFlow("127.0.0.1")
        override val hueApiKey: Flow<String?>
            get() = _hueApiKey

        override suspend fun saveIp(ip: String?) {
            ip?.let { _ipFlow.value = it }
        }

        override suspend fun savePort(port: Int?) {
            port?.let { _portFlow.value = it }

        }

        override suspend fun saveHueApiKey(hueApiKey: String?) {
            hueApiKey?.let { _hueApiKey.value = it }
        }
    }

    // 2. Stateful MockEngine handling Groups, Lights, and Switches
    private val mockEngine = MockEngine { request ->
        val url = request.url.encodedPath
        val method = request.method

        when {
            // --- Groups ---
            url.endsWith("/groups") && method == HttpMethod.Post -> {
                val group =
                    json.decodeFromString<Group>(request.body.toByteArray().decodeToString())
                mockGroups.removeAll { it.id == group.id }
                mockGroups.add(group)
                respondOk()
            }

            // --- Lights ---
            url.endsWith("/lights") && method == HttpMethod.Post -> {
                val light =
                    json.decodeFromString<Light>(request.body.toByteArray().decodeToString())
                mockLights.removeAll { it.uid == light.uid }
                mockLights.add(light)
                respondOk()
            }

            url.endsWith("/lights") && method == HttpMethod.Delete -> {
                val uid = request.url.parameters["uid"]
                // Note: LightsDatabaseManager::deleteLight uses both uid and groupId
                mockLights.removeAll { it.uid == uid }
                respondOk()
            }

            // --- Switches ---
            url.endsWith("/switches") && method == HttpMethod.Post -> {
                val sw = json.decodeFromString<Switch>(request.body.toByteArray().decodeToString())
                mockSwitches.removeAll { it.uid == sw.uid }
                mockSwitches.add(sw)
                respondOk()
            }

            url.endsWith("/switches") && method == HttpMethod.Delete -> {
                val uid = request.url.parameters["uid"]
                mockSwitches.removeAll { it.uid == uid }
                respondOk()
            }

            url.endsWith("/switches") && method == HttpMethod.Get -> {
                val groupId = request.url.parameters["groupId"]?.toLong()
                val filtered =
                    if (groupId != null) mockSwitches.filter { it.groupId == groupId } else mockSwitches
                respondJson(json.encodeToString(filtered))
            }

            else -> respond("Not Found", HttpStatusCode.NotFound)
        }
    }

    private fun MockRequestHandleScope.respondOk() = respond(
        content = """{"status":"ok"}""",
        status = HttpStatusCode.OK,
        headers = headersOf(HttpHeaders.ContentType, "application/json")
    )

    private fun MockRequestHandleScope.respondJson(content: String) = respond(
        content = content,
        status = HttpStatusCode.OK,
        headers = headersOf(HttpHeaders.ContentType, "application/json")
    )

    @Test
    fun `test usecase handles switches and lights correctly`() = runTest {
        // Dependencies
        val client = HttpClient(mockEngine) { install(ContentNegotiation) { json(json) } }
        val service = ServerService(client)

        val repository = ServerRepository(service, mockSettingRepo)
        val useCase = SaveItemsUseCase(repository)

        // Data Setup
        val groupId = 1L
        val group = Group(groupId, "Living Room", 0.8f, 0.5f, 0.5f, 300)

        // Items to add/update
        val newLight = Light("L1", "Bulb", StateMode.ON, LightType.HUE, 1.0f, 0f, 0f, 300, groupId)
        val newSwitch = Switch("S1", "Wall Switch", groupId)

        // Item to delete
        val oldSwitch = Switch("S_OLD", "Old Switch", groupId)
        mockSwitches.add(oldSwitch)

        // Execute UseCase
        val result = useCase.invoke(
            group = group,
            upsertItems = listOf(newLight, newSwitch), // Mix of types
            deletedItems = listOf(oldSwitch)
        )

        // Assertions
        assertTrue(result.isSuccess)

        // Verify Server State
        val serverSwitches = repository.getSwitchesByGroup(groupId)
        assertEquals(1, serverSwitches.size)
        assertEquals("S1", serverSwitches.first().uid)
        assertTrue(serverSwitches.none { it.uid == "S_OLD" }) // Ensure deleteSwitch worked

        assertEquals(1, mockLights.size)
        assertEquals("L1", mockLights.first().uid)
    }
}