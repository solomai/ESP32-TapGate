using System.Threading.Tasks;
using Microsoft.Maui.Storage;

namespace ProtocolTest.Service.Cryptographer.Impl
{
    public static class PemStorage
    {
        private const string PrivateKeySlot      = "tapgate.private.pem";
        private const string PublicKeySlot       = "tapgate.public.pem";
        private const string DevicePublicKeySlot = "tapgate.device.public.pem";

        public static async Task SaveKeysAsync(string privatePem, string publicPem)
        {
            // SecureStorage encrypts at rest using OS keystores.
            await SecureStorage.SetAsync(PrivateKeySlot, privatePem);
            await SecureStorage.SetAsync(PublicKeySlot, publicPem);
        }

        public static async Task<(string? privatePem, string? publicPem, string? devicePublicPem)> LoadKeysAsync()
        {
            var priv = await SecureStorage.GetAsync(PrivateKeySlot);
            var pub = await SecureStorage.GetAsync(PublicKeySlot);
            var devicepub = await SecureStorage.GetAsync(DevicePublicKeySlot);
            return (priv, pub, devicepub);
        }

        public static void DeleteKeys()
        {
            SecureStorage.Remove(PrivateKeySlot);
            SecureStorage.Remove(PublicKeySlot);
            SecureStorage.Remove(DevicePublicKeySlot);
        }
    }
}
