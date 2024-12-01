#include <glib.h>
#include <gio/gio.h>
#include <stdio.h>
#include <string.h>

#define CMD_CHARACTERISTIC_UUID "12345678-1234-5678-1234-56789abcdef2"
#define DATA_CHARACTERISTIC_UUID "12345678-1234-5678-1234-56789abcdef3"

static char scan_result[128] = "No scan yet"; // Storage for the response

// Command write callback
static gboolean on_command_write(GDBusConnection *connection,
                                 const gchar *sender,
                                 const gchar *object_path,
                                 const gchar *interface_name,
                                 const gchar *method_name,
                                 GVariant *parameters,
                                 GDBusMethodInvocation *invocation,
                                 gpointer user_data) {
    const gchar *command_data;
    g_variant_get(parameters, "(ay)", &command_data);

    printf("Received command: %s\n", command_data);

    if (strcmp(command_data, "SCAN_FINGERPRINT") == 0) {
        printf("Performing fingerprint scan...\n");
        snprintf(scan_result, sizeof(scan_result), "Fingerprint: ID-12345");
    } else {
        snprintf(scan_result, sizeof(scan_result), "Unknown command");
    }

    g_dbus_method_invocation_return_value(invocation, NULL); // Acknowledge write
    return TRUE;
}

// Data read callback
static gboolean on_data_read(GDBusConnection *connection,
                             const gchar *sender,
                             const gchar *object_path,
                             const gchar *interface_name,
                             const gchar *method_name,
                             GVariant *parameters,
                             GDBusMethodInvocation *invocation,
                             gpointer user_data) {
    printf("Read request received\n");
    GVariant *response = g_variant_new_fixed_array(G_VARIANT_TYPE_BYTE, scan_result, strlen(scan_result), 1);
    g_dbus_method_invocation_return_value(invocation, g_variant_new_tuple(&response, 1));
    return TRUE;
}

// Main program entry
int main(int argc, char *argv[]) {
    GError *error = NULL;
    GMainLoop *loop = g_main_loop_new(NULL, FALSE);
    GDBusConnection *connection = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error);

    if (error != NULL) {
        g_printerr("Failed to connect to system bus: %s\n", error->message);
        g_clear_error(&error);
        return 1;
    }

    // Define characteristics
    GDBusNodeInfo *cmd_char_info = g_dbus_node_info_new_for_xml(
        "<node>"
        "  <interface name='org.bluez.GattCharacteristic1'>"
        "    <property name='UUID' type='s' access='read'/>"
        "    <property name='Flags' type='as' access='read'/>"
        "    <method name='WriteValue'>"
        "      <arg type='ay' name='value' direction='in'/>"
        "    </method>"
        "  </interface>"
        "</node>",
        NULL);

    GDBusNodeInfo *data_char_info = g_dbus_node_info_new_for_xml(
        "<node>"
        "  <interface name='org.bluez.GattCharacteristic1'>"
        "    <property name='UUID' type='s' access='read'/>"
        "    <property name='Flags' type='as' access='read'/>"
        "    <method name='ReadValue'>"
        "      <arg type='a{sv}' name='options' direction='in'/>"
        "      <arg type='ay' name='value' direction='out'/>"
        "    </method>"
        "  </interface>"
        "</node>",
        NULL);

    // Register the command characteristic
    guint cmd_char_id = g_dbus_connection_register_object(
        connection,
        "/org/example/command_char",
        cmd_char_info->interfaces[0],
        &(GDBusInterfaceVTable){.method_call = on_command_write},
        NULL,
        NULL,
        &error);

    if (cmd_char_id == 0) {
        g_printerr("Failed to register command characteristic: %s\n", error->message);
        g_clear_error(&error);
        return 1;
    }

    // Register the data characteristic
    guint data_char_id = g_dbus_connection_register_object(
        connection,
        "/org/example/data_char",
        data_char_info->interfaces[0],
        &(GDBusInterfaceVTable){.method_call = on_data_read},
        NULL,
        NULL,
        &error);

    if (data_char_id == 0) {
        g_printerr("Failed to register data characteristic: %s\n", error->message);
        g_clear_error(&error);
        return 1;
    }

    printf("GATT server is running...\n");
    g_main_loop_run(loop);

    // Cleanup
    g_dbus_connection_unregister_object(connection, cmd_char_id);
    g_dbus_connection_unregister_object(connection, data_char_id);
    g_dbus_node_info_unref(cmd_char_info);
    g_dbus_node_info_unref(data_char_info);
    g_object_unref(connection);
    g_main_loop_unref(loop);

    return 0;
}
